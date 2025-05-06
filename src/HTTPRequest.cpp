#include "../include/HTTPRequest.hpp"
#include "../include/Utils.hpp"
#include "../include/webserver.hpp"
#include <algorithm>
#include "../include/Logger.hpp"
#include <sstream>
#include <iostream>
#include <cctype>
#include <cstdlib>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <algorithm>

HTTPRequest::HTTPRequest() : _client(NULL),_statusCode(0),_state(INIT),_parsePosition(0),_contentLength(0),_isChunked(false),_keepAlive(false), _chunkSize(-1),_matchedLocation(NULL), _finalLocation(NULL)
{
}

HTTPRequest::HTTPRequest(Client* client) : 
	_client(client),
	_statusCode(0),
	_state(INIT),
	_parsePosition(0),
	_contentLength(0),
	_method(""),
	_uri(""),
	_path(""),
	_query(""),
	_protocol(""),
	_body(),
	_tmpHeaderKey(""),
	_tmpHeaderValue(""),
	_request("")
{}


HTTPRequest::HTTPRequest(const HTTPRequest& other) 
{
	*this = other;
}


bool HTTPRequest::shouldKeepAlive() const
{
	return _keepAlive;
}
const LocationConfig* HTTPRequest::getMatchedLocation() const 
{
	return _matchedLocation;
}

const LocationConfig* HTTPRequest::getFinalLocation() const 
{
	return _finalLocation;
}

HTTPRequest& HTTPRequest::operator=(const HTTPRequest& other) 
{
	if (this != &other) 
	{
		_client = other._client;
		_statusCode = other._statusCode;
		_state = other._state;
		_parsePosition = other._parsePosition;
		_contentLength = other._contentLength;
		_method = other._method;
		_uri = other._uri;
		_path = other._path;
		_query = other._query;
		_protocol = other._protocol;
		_body = other._body;
		_tmpHeaderKey = other._tmpHeaderKey;
		_tmpHeaderValue = other._tmpHeaderValue;
		_request = other._request;
		_headers = other._headers;
		_contentType = other._contentType;
		_boundary = other._boundary;
		_isChunked = other._isChunked;
		_host = other._host;
		_keepAlive = other._keepAlive;
		_chunkSize = other._chunkSize;
	}
	return *this;
}

HTTPRequest::~HTTPRequest() 
{
}

const std::string&	HTTPRequest::getUri() const
{
	return _uri;
}

const std::string&	HTTPRequest::getBody() const
{
	return _body;
}

const std::string& HTTPRequest::getMethod() const 
{
	return _method;
}

const std::string& HTTPRequest::getPath() const 
{
	return _path;
}

const std::string& HTTPRequest::getHeaderValue(const std::string& key) const 
{
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	if (it != _headers.end()) 
		return it->second;
	static std::string empty;
	return empty;
}

int HTTPRequest::getState() const 
{
	return _state;
}

int HTTPRequest::getStatusCode() const 
{
	return _statusCode;
}

void HTTPRequest::setStatusCode(int code) 
{
	_statusCode = code;
	_state = FINISH;
}

void HTTPRequest::setState(ParseState state) 
{
	_state = state;
}

bool HTTPRequest::isUriCharacter(char c)
{
	return std::isalnum(c) ||
		c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
		c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
		c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

void HTTPRequest::parse(const std::string& rawdata)
{
	if (_state == FINISH || rawdata.empty()) 
		return;

	_request.append(rawdata);

	parseRequestLine();
	parseRequestHeader();
	parseRequestBody();

	if (_statusCode == 0 && _state == FINISH) 
		_statusCode = 200;
	debugPrintRequest();
}

void HTTPRequest::parseRequestHeader() 
{
	if (_state < HEADERS_INIT || _state > HEADERS_END)
		return;
	if (_state == HEADERS_INIT)
		setState(HEADER_KEY);
	if (_state == HEADER_KEY) 
		parseHeadersKey();
	if (_state == HEADER_VALUE) 
		parseHeadersValue();
	if (_state == HEADERS_END) 
		checkHeadersEnd();
}

void HTTPRequest::parseRequestLine() 
{
	if (_state > LINE_END) 
		return;

	if (_state == INIT) 
		_state = LINE_METHOD;

	if (_state == LINE_METHOD) 
		parseMethod();
	if (_state == LINE_URI) 
		parseUri();
	if (_state == LINE_VERSION) 
		parseVersion();
	if (_state == LINE_END) 
		checkLineEnd();
}

void HTTPRequest::parseMethod() 
{
	size_t i = 0;
	size_t rawSize = _request.size();
	bool found = false;

	// Extract method name until space
	while (i < rawSize) 
	{
		if (_request[i] == ' ') 
		{
			found = true;
			break;
		}
		if (!std::isalpha(_request[i]))
			return (void)setStatusCode(400);
		_method += _request[i];
		i++;
	}
	_request.erase(0, found ? i + 1 : i);
	if (found) 
	{
		if (_method.empty())
			return (void)setStatusCode(400);
		if (_method != "GET" && _method != "POST" && _method != "DELETE")
			setStatusCode(400);

		setState(LINE_URI);
	}
}

void HTTPRequest::parseUri() 
{
	size_t i = 0;
	size_t rawSize = _request.size();
	bool found = false;

	while (i < rawSize && _uri.empty() && (_request[i] == ' ' || _request[i] == '\t'))
		i++;
	while (i < rawSize) {
		if (_request[i] == ' ') 
		{
			found = true;
			break;
		}
		if (!std::isprint(_request[i]))
			return (void)setStatusCode(400);
		_uri += _request[i];
		i++;
	}
	_request.erase(0, found ? i + 1 : i);
	if (found) 
	{
		if (_uri.empty())
			return (void)setStatusCode(400);
		if (Utils::urlDecode(_uri) == -1)
			return (void)setStatusCode(400);
		size_t pos = _uri.find('?');
		if (pos != std::string::npos) 
		{
			_path = _uri.substr(0, pos);
			_query = _uri.substr(pos + 1);
		} 
		else 
			_path = _uri;
		findLocation();
		setState(LINE_VERSION);
	}
}

// Helper to find location by path
const LocationConfig* HTTPRequest::findLocationByPath(const std::string& path) const 
{
	const std::map<std::string, LocationConfig>& locations = 
		_client->getServer()->getLocations();

	// 1. Check exact match
	std::map<std::string, LocationConfig>::const_iterator it = locations.find(path);
	if (it != locations.end()) 
		return &it->second;

	// 2. Find best prefix match
	std::string bestMatch;
	for (it = locations.begin(); it != locations.end(); ++it) 
	{
		if (path.compare(0, it->first.length(), it->first) == 0 && it->first.length() > bestMatch.length()) 
			bestMatch = it->first;
	}

	if (!bestMatch.empty()) 
		return &locations.find(bestMatch)->second;

	// 3. Fallback to default location
	it = locations.find("/");
	return (it != locations.end()) ? &it->second : NULL;
}

void HTTPRequest::findLocation() 
{
	_matchedLocation = findLocationByPath(_path);
	_finalLocation = _matchedLocation;

	// Follow redirects if any
	if (_matchedLocation && _matchedLocation->is_location_have_redirection()) 
		_finalLocation = findLocationByPath(_matchedLocation->getRedirectPath());
}


void HTTPRequest::parseVersion() 
{
	size_t i = 0;
	size_t rawSize = _request.size();
	bool found = false;

	// Skip leading whitespace
	while (i < rawSize && _protocol.empty() && (_request[i] == ' ' || _request[i] == '\t'))
		i++;

	while (i < rawSize) 
	{
		char c = _request[i];
		if (!(c == 'H' || c == 'T' || c == 'P' || c == '/' || c == '.' || std::isdigit(c))) 
		{
			found = true;
			break;
		}
		_protocol += c;
		i++;
	}
	_request.erase(0, i);
	if (found) 
	{
		if (_protocol.empty())
			return (void)setStatusCode(400);
		if (_protocol != "HTTP/1.1")
			return (void)setStatusCode(505);
		setState(LINE_END);
	}
}

void HTTPRequest::checkLineEnd() 
{
	// Remove leading whitespace
	_request.erase(0, _request.find_first_not_of(" \t"));
	if (_request.empty())
		return;
	if (_request[0] == '\n') 
	{
		_request.erase(0, 1);
		setState(HEADERS_INIT);
		return;
	}

	if (_request[0] == '\r') 
	{
		if (_request.size() < 2)
			return;
		if (_request[1] == '\n') 
		{
			_request.erase(0, 2);
			setState(HEADERS_INIT);
			return;
		}
		setStatusCode(400);
		return;
	}
	setStatusCode(400);
}


void HTTPRequest::parseHeadersKey() 
{
	// Check for end of headers section
	if (!_request.empty() && (_request[0] == '\r' || _request[0] == '\n')) 
	{
		if (_request[0] == '\n') 
		{
			_request.erase(0, 1);

			if (!_tmpHeaderKey.empty())
				return (void)setStatusCode(400);

			setState(BODY_INIT);
			return;
		}

		if (_request.size() < 2)
			return;

		if (_request[1] == '\n') 
		{
			_request.erase(0, 2);
			if (!_tmpHeaderKey.empty())
				return (void)setStatusCode(400);
			setState(BODY_INIT);
			return;
		}
		setStatusCode(400);
		return;
	}

	size_t i = 0;
	size_t rawSize = _request.size();
	bool found = false;

	// Extract header key until colon
	while (i < rawSize) 
	{
		if (_request[i] == ' ' || _request[i] == '\t')
			return setStatusCode(400);

		if (_request[i] == ':') 
		{
			found = true;
			break;
		}

		if (!std::isalnum(_request[i]) && _request[i] != '-' && _request[i] != '_')
			return setStatusCode(400);

		_tmpHeaderKey += _request[i];
		i++;
	}
	_request.erase(0, found ? i + 1 : i);
	if (found) 
	{
		if (_tmpHeaderKey.empty())
			return (void)setStatusCode(400);
		// Remove whitespace from header key
		_tmpHeaderKey.erase(std::remove_if(_tmpHeaderKey.begin(), _tmpHeaderKey.end(), ::isspace), _tmpHeaderKey.end());
		setState(HEADER_VALUE);
	}
}

void HTTPRequest::parseHeadersValue() 
{
	size_t i = 0;
	size_t rawSize = _request.size();
	bool found = false;

	// Skip leading whitespace
	while (i < rawSize && _tmpHeaderValue.empty() && (_request[i] == ' ' || _request[i] == '\t'))
		i++;
	// Extract header value until non-printable char
	while (i < rawSize) 
	{
		if (!std::isprint(_request[i])) 
		{
			found = true;
			break;
		}

		_tmpHeaderValue += _request[i];
		i++;
	}
	_request.erase(0, i);
	if (found) 
	{
		if (_tmpHeaderValue.empty())
			return (setStatusCode(400));
		if (_headers.find(_tmpHeaderKey) != _headers.end())
		{
			if (_tmpHeaderKey == "Content-Length" || _tmpHeaderKey == "Host" || _tmpHeaderKey == "Content-Type") 
				return setStatusCode(400); 
			else 
				_headers[_tmpHeaderKey] += ", " + _tmpHeaderValue;
		}
		else 
			_headers[_tmpHeaderKey] = _tmpHeaderValue;
		_tmpHeaderKey.clear();
		_tmpHeaderValue.clear();
		setState(HEADERS_END);
	}
}

void HTTPRequest::checkHeadersEnd() 
{
	_request.erase(0, _request.find_first_not_of(" \t"));
	if (_request.empty())
		return;
	if (_request[0] == '\n') 
	{
		_request.erase(0, 1);
		setState(HEADER_KEY);
		parseRequestHeader();
		return;
	}
	if (_request[0] == '\r') 
	{
		if (_request.size() < 2)
			return;
		if (_request[1] == '\n') 
		{
			_request.erase(0, 2);
			setState(HEADER_KEY);
			parseRequestHeader();
			return;
		}
		setStatusCode(400);
		return;
	}
	setStatusCode(400);
}

int	HTTPRequest::checkCgi(void)
{
	return (0);
} 

// int	HTTPRequest::checkMethod(void)
// {
// 	LocationConfig location = findMatchingLocation(this->getPath());
// 	if (location.isMethodAllowed(this->_method))
// 		return (0);
// 	this->setStatusCode(405);
// 	return (-1);
// }

int	HTTPRequest::checkClientMaxBodySize(void)
{
	if (this->_headers.find("Content-Length") != this->_headers.end())
		this->_contentLength = Utils::stringToSizeT(this->_headers["Content-Length"]);
	if (this->_contentLength > this->_client->getServer()->getClientMaxBodySize())
	{
		this->setStatusCode(413);
		return -1;
	}
	return 0;
}

int	HTTPRequest::checkTransferEncoding(void)
{
	if (this->_headers.find("Transfer-Encoding") != this->_headers.end())
	{
		if (this->_headers["Transfer-Encoding"] == "chunked")
			this->_isChunked = true;
	}
	return (0);
}


void HTTPRequest::parseRequestBody() 
{
	if (_state < BODY_INIT) 
		return;

	// if (this->_isChunked == true)
	// 	this->parseChunkBody();
	
	// Store the entire remaining buffer as the body
	_body = _request;
	_request.clear();
	_state = FINISH;
}

void HTTPRequest::debugPrintRequest() const 
{
	std::cout << "------------------------------" << std::endl;
	std::cout << "method  : " << _method << std::endl;
	std::cout << "_uri  : " << _uri << std::endl;
	std::cout << "_path  : " << _path << std::endl;
	std::cout << "_query  : " << _query << std::endl;
	std::cout << "_protocol  : " << _protocol << std::endl;
	std::cout << "==== Headers ====" << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); 
			it != _headers.end(); ++it) {
		std::cout << it->first << ": " << it->second << std::endl;
	}
	std::cout << "----------------_body----------------" << std::endl;	std::cout << _body << std::endl;
	std::cout << _body << std::endl;
	std::cout << "----------------" << std::endl;
}
