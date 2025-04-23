#include "../include/HTTPRequest.hpp"
#include "../include/Utils.hpp"
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

HTTPRequest::HTTPRequest(Client* client) : _client(client), _statusCode(0), _state(INIT), _parsePosition(0), _contentLength(0) {}

HTTPRequest::~HTTPRequest() 
{
}

// Getters
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

const std::string&	HTTPRequest::getUri() const
{
	return _uri;
}

const std::string&	HTTPRequest::getBody() const
{
	return _body;
}

// Main parsing function
void HTTPRequest::parse(const std::string& rawdata) 
{
     if (_state == FINISH || rawdata.empty()) 
         return;
     _request.append(rawdata);

    parseRequestLine();
    parseRequestHeader();
    parseRequestBody();
    std::cout << "------------------------------" << std::endl;
    std::cout << "method  : " << _method << std::endl;
    std::cout << "_uri  : " << _uri << std::endl;
    std::cout << "_path  : " << _path << std::endl;
    std::cout << "_query  : " << _query << std::endl;
    std::cout << "_protocol  : " << _protocol << std::endl;
    std::cout << "==== Headers ====" << std::endl;
    for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it)
        std::cout << it->first << ": " << it->second << std::endl;
    std::cout << "----------------_body----------------" << std::endl;
    std::cout << _body << std::endl;
    std::cout << "----------------" << std::endl;

}

// Request line parsing
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
	while (i < _request.size() && _request[i] != ' ') 
	{
		if (!isalpha(_request[i])) 
			return (setStatusCode(400));
		_method += _request[i++];
	}

	if (i == _request.size()) 
	{
		_request.clear();
		return;
	}
	_request.erase(0, i + 1);
	if (_method.empty()) return setStatusCode(400);
	if (_method != "GET" && _method != "POST" && _method != "DELETE") 
		return setStatusCode(400);
	_state = LINE_URI;
}

void HTTPRequest::parseUri() 
{
	size_t i = 0;
	while (i < _request.size() && _request[i] != ' ') 
	{
		if (!isprint(_request[i])) 
			return (setStatusCode(400));
		_uri += _request[i++];
	}
	if (i == _request.size()) 
	{
		_request.clear();
		return;
	}

	_request.erase(0, i + 1);
	if (_uri.empty()) return 
		setStatusCode(400);
	if (Utils::urlDecode(_uri) == -1) 
		return (setStatusCode(400));

	size_t pos = _uri.find('?');
	if (pos != std::string::npos) 
	{
		_path = _uri.substr(0, pos);
		_query = _uri.substr(pos + 1);
	} 
	else 
		_path = _uri;
	_state = LINE_VERSION;
}

void HTTPRequest::parseVersion() 
{
	size_t i = 0;
	while (i < _request.size() && _request[i] != '\r' && _request[i] != '\n') 
	{
		char c = _request[i];
		if (c != 'H' && c != 'T' && c != 'P' && c != '/' && c != '.' && !isdigit(c)) 
			break;
		_protocol += c;
		i++;
	}

	_request.erase(0, i);
	if (_protocol.empty()) return setStatusCode(400);
	if (_protocol != "HTTP/1.1") return setStatusCode(505);
	_state = LINE_END;
}

void HTTPRequest::checkLineEnd() 
{
	Utils::skipWhitespace(_request);
	if (_request.empty()) return;

	if (_request[0] == '\n') 
	{
		_request.erase(0, 1);
		_state = HEADERS_INIT;
	} 
	else if (_request[0] == '\r') 
	{
		if (_request.size() < 2) 
			return;
		if (_request[1] == '\n') 
		{
			_request.erase(0, 2);
			_state = HEADERS_INIT;
		} 
		else
			setStatusCode(400);
	} 
	else 
		setStatusCode(400);
}

// Header parsing
void HTTPRequest::parseRequestHeader() 
{
	while (_state >= HEADERS_INIT && _state <= HEADERS_END)
	{
		if (_state == HEADERS_INIT)
			_state = HEADERS_KEY;
		else if (_state == HEADERS_KEY)
			parseHeaderKey();
		else if (_state == HEADERS_VALUE)
			parseHeaderValue();
		else if (_state == HEADERS_END)
			checkHeaderEnd();

		if (_statusCode != 0)
			break;
	}
}

void HTTPRequest::parseHeaderKey() 
{
	if (_request.empty()) 
		return;

	if (_request[0] == '\r' || _request[0] == '\n') 
	{
		if (_request[0] == '\n') 
		{
			_request.erase(0, 1);
			if (!_tmpHeaderKey.empty()) return setStatusCode(400);
			_state = BODY_INIT;
			return;
		}

		if (_request.size() < 2) 
			return;
		if (_request[1] == '\n') 
		{
			_request.erase(0, 2);
			if (!_tmpHeaderKey.empty()) 
				return (setStatusCode(400));
			_state = BODY_INIT;
			return;
		}
		return setStatusCode(400);
	}

	size_t i = 0;
	bool foundColon = false;
	while (i < _request.size()) 
	{
		char c = _request[i];
		if (c == ' ' || c == '\t') 
			return setStatusCode(400);
		if (c == ':')
		{
			foundColon = true;
			break;
		}
		if (!isalnum(c) && c != '-' && c != '_') 
			return setStatusCode(400);
		_tmpHeaderKey += c;
		i++;
	}

	_request.erase(0, foundColon ? i + 1 : i);
	if (foundColon) 
	{
		if (_tmpHeaderKey.empty()) return setStatusCode(400);
		_state = HEADERS_VALUE;
	}
}

void HTTPRequest::parseHeaderValue() 
{
	size_t i = 0;
	bool foundEnd = false;
	while (i < _request.size()) 
	{
		char c = _request[i];
		if (!isprint(c)) 
		{
			foundEnd = true;
			break;
		}
		_tmpHeaderValue += c;
		i++;
	}
	_request.erase(0, i);
	if (foundEnd)
	{
		Utils::trim(_tmpHeaderValue);
		if (_headers.find(_tmpHeaderKey) != _headers.end()) 
			return (setStatusCode(400));

		_headers[_tmpHeaderKey] = _tmpHeaderValue;
		_tmpHeaderKey.clear();
		_tmpHeaderValue.clear();
		_state = HEADERS_END;
	}
}

void HTTPRequest::checkHeaderEnd() 
{
	Utils::skipWhitespace(_request);
	if (_request.empty()) 
		return;

	if (_request[0] == '\n') 
	{
		_request.erase(0, 1);
		_state = HEADERS_KEY;
	} 
	else if (_request[0] == '\r') 
	{
		if (_request.size() < 2) 
			return;
		if (_request[1] == '\n') 
		{
			_request.erase(0, 2);
			_state = HEADERS_KEY;
		} 
		else 
			setStatusCode(400);
	} 
	else 
		setStatusCode(400);
}

// Body parsing
void HTTPRequest::parseRequestBody() 
{
	if (_state != BODY_INIT && _state != BODY_PROCESS) 
		return;

	std::map<std::string, std::string>::iterator it = _headers.find("Transfer-Encoding");
	if (it != _headers.end()) 
	{
		if (it->second.find("chunked") != std::string::npos) 
			return setStatusCode(501); // Not Implemented
		return setStatusCode(400); // Bad Request
	}

	if (_method != "POST") 
	{
		_state = FINISH;
		return;
	}

	it = _headers.find("Content-Length");
	if (it == _headers.end()) 
		return setStatusCode(400); // Bad Request

	if (_contentLength == 0) 
	{
		char* end;
		_contentLength = strtoul(it->second.c_str(), &end, 10);
		if (*end != '\0' || _contentLength == 0) 
			return (setStatusCode(400));
	}

	size_t remaining = _contentLength - _body.size();
	size_t toRead = std::min(remaining, _request.size());

	_body.append(_request.substr(0, toRead));
	_request.erase(0, toRead);

	if (_body.size() == _contentLength) 
	{
		if (!_request.empty()) 
			return (setStatusCode(400)); // Extra data after body
		_state = FINISH;
	} 
	else 
		_state = BODY_PROCESS;
}

void HTTPRequest::setState(ParseState state) 
{ 
	_state = state; 
}

void HTTPRequest::setStatusCode(int code) 
{ 
	_statusCode = code; _state = FINISH;
}


void HTTPRequest::print()
{
	std::cout << "Methode : " << _method << std::endl;
	std::cout << "URI : " << _uri << std::endl;
	std::cout << "Protocol : " << _protocol << std::endl;
	for (std::map<std::string, std::string>::iterator it = _headers.begin()
		; it != _headers.end(); it++)
		std::cout << it->first << " : " << it->second << std::endl;
}
