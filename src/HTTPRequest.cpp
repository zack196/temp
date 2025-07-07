#include "../include/HTTPRequest.hpp"
#include "../include/Logger.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/LocationConfig.hpp"
#include "../include/Utils.hpp"
#include <bits/types/locale_t.h>
#include <cctype>
#include <cmath>
#include <csetjmp>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <netdb.h>
#include <string>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>


HTTPRequest::HTTPRequest(std::vector<ServerConfig>& servers)
	: _server(servers[0]),
	 _servers(servers),
	_statusCode(200),
	_state(INIT),
	_contentLength(0),
	_method(""),
	_uri("/"),
	_path(""),
	_query(""),
	_protocol("HTTP/1.1"),
	_bodyBuffer(""),
	_headers(),
	_contentType(""),
	_resource(""),
	_isChunked(false),
	_keepAlive(true),
	_chunkSize(-1),
	_length(0),
	_boundary("--"),
	_multipartState(PART_HEADER),
	_chunkState(CHUNK_SIZE),
	_bodyFile(""),
	_totalBodySize(0)
{
}


HTTPRequest::~HTTPRequest() 
{
	if (_uploadFile.is_open())
		_uploadFile.close();
	if (_body.is_open())
	{
		std::remove(_bodyFile.c_str());
		_body.close();
	}
}

void HTTPRequest::setClientfd(int fd)
{
	_client_fd = fd;
}

bool HTTPRequest::keepAlive() const 
{
	return _keepAlive;
}

const std::map<std::string, std::string>& HTTPRequest::getHeaders() const 
{
	return _headers;
}

bool HTTPRequest::isComplete() const 
{
	return (_state == FINISH || _state == ERROR);
}


bool HTTPRequest::hasCgi()
{
	if (_location.hasCgi())
		return true;

	return false;
}

const std::string& HTTPRequest::getUri() const 
{
	return _uri;
}

const std::string& HTTPRequest::getProtocol() const 
{
	return _protocol;
}

const std::string& HTTPRequest::getQuery() const 
{
	return _query;
}

const std::string& HTTPRequest::getBody() const 
{
	return _bodyBuffer;
}

const std::string& HTTPRequest::getMethod() const 
{
	return _method;
}

const std::string& HTTPRequest::getResource() const 
{
	return _resource;
}

const std::string& HTTPRequest::getPath() const 
{
	return _path;
}

int HTTPRequest::getState() const 
{
	return _state;
}

int HTTPRequest::getStatusCode() const 
{
	return _statusCode;
}

const ServerConfig& HTTPRequest::getServer() const 
{
    return _server;
}

size_t HTTPRequest::getContentLength() const 
{
	return _contentLength;
}

const std::string& HTTPRequest::getHeader(const std::string& key) const 
{
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	static std::string empty;
	return (it != _headers.end()) ? it->second : empty;
}

void HTTPRequest::setStatusCode(int code) 
{
	_statusCode = code;
}

void HTTPRequest::setState(ParseState state) 
{
	_state = state;
}

std::string HTTPRequest::getQueryParameter(const std::string& key) const 
{
	std::map<std::string, std::string>::const_iterator it = _queryParameters.find(key);
	if (it != _queryParameters.end()) 
		return it->second;
	return "";
}

const LocationConfig& HTTPRequest::getLocation() const 
{
	return _location;
}

void HTTPRequest::parseRequest(std::string& data) 
{
	if (data.empty())
		return;

	if (_state == INIT)
		_state = METHOD;
	if (_state == METHOD)
		parseMethod(data);
	if (_state == URI)
		parseUri(data);
	if (_state == PROTOCOL)
		parseProtocol(data);
	if (_state == HEADER)
		parseHeaders(data);
	if (_state == BODY_INIT)
		parseBody();
	if (_state == CGI)
		writeBodyToFile(data);
	if (_state == CHUNKED)
		parseChunkBody(data);
	if (_state == MULTIPART)
		parseMultipartBody(data);

	if (_state == FINISH || _state == ERROR)
		return;
}

void HTTPRequest::parseMethod(std::string& data) 
{
	size_t space_pos = data.find(' ');
	if (space_pos == std::string::npos)
		return;
	_method = data.substr(0, space_pos);

	if (_method.empty() || !Utils::isValidMethodToken(_method)) 
	{
		setState(ERROR);
		setStatusCode(400);
		return;
	}

	if (_method != "GET" && _method != "POST" && _method != "DELETE") 
	{
		setState(ERROR);
		setStatusCode(501);
		return;
	}

	data.erase(0, space_pos + 1);
	setState(URI);
}



void HTTPRequest::parseUri(std::string& data) 
{
	size_t space_pos = data.find(' ');
	if (space_pos == std::string::npos)
		return;
	_uri = data.substr(0, space_pos);
	if (_uri.size() > 8192) 
	{
		setState(ERROR);
		setStatusCode(414);
		return;
	}
	if (_uri.empty() || !Utils::isValidUri(_uri)) 
	{
		setState(ERROR);
		setStatusCode(400);
		return;
	}
	if (Utils::urlDecode(_uri) == -1) 
	{
		setState(ERROR);
		setStatusCode(400);
		return;
	}
	size_t query_pos = _uri.find('?');
	if (query_pos != std::string::npos) 
	{
		_path = _uri.substr(0, query_pos);
		_query = _uri.substr(query_pos + 1);
	} 
	else 
		_path = _uri;
	data.erase(0, space_pos + 1);

	setState(PROTOCOL);
}

void HTTPRequest::parseProtocol(std::string& data) 
{
	size_t pos = data.find("\r\n");
	if (pos == std::string::npos) 
		return;

	size_t line_len = 2;
	_protocol = Utils::trim(data.substr(0, pos));
	if (_protocol != "HTTP/1.1") 
	{
		setState(ERROR);
		setStatusCode(400);
		return;
	}

	data.erase(0, pos + line_len);
	setState(HEADER);
}

void HTTPRequest::parseHeaders(std::string& data)
{
	if (data.empty())
		return;

	size_t line_end = data.find("\r\n");
	if (line_end == std::string::npos)
		return;

	size_t sep_len = 2;

	std::string line = data.substr(0, line_end);

	if (line.empty()) 
	{
		if (_headers.empty() || _headers.find("host") == _headers.end()) 
			return (setState(ERROR), setStatusCode(400));
		else 
			setState(BODY_INIT);
		data.erase(0, line_end + sep_len);
		return;
	}

	size_t colon_pos = line.find(':');
	if (colon_pos == std::string::npos)
	{
		setState(ERROR);
		setStatusCode(400);
		return;
	}

	std::string key = line.substr(0, colon_pos);
	std::string value = Utils::trim(line.substr(colon_pos + 1));

	for (size_t i = 0; i < key.size(); ++i)
		key[i] = std::tolower(key[i]);

	if (!Utils::isValidHeaderKey(key) || !Utils::isValidHeaderValue(value)) 
		return (setState(ERROR), setStatusCode(400));

	if (_headers.find(key) != _headers.end()) 
	{
		if (key == "host" || key == "content-length" || key == "transfer-encoding" || key == "connection")
			return (setState(ERROR), setStatusCode(400));
		else if (key == "cookie") 
			_headers[key] += "; " + value;
		else 
			_headers[key] = value;
	} 
	else 
		_headers[key] = value;

	data.erase(0, line_end + sep_len);
	setState(HEADER);
	parseRequest(data);
}

const std::string& HTTPRequest::getBodyfile() const
{
	return _bodyFile;
}

bool HTTPRequest::validateCgi()
{
	if (!hasCgi())
		return true;
	try 
	{
		_bodyFile = Utils::createTempFile("body_", _server.getClientBodyTmpPath());
		_body.open(_bodyFile.c_str(), std::ios::binary);
		if (!_body.is_open())
		{
			LOG_ERROR("Failed to open CGI body file: " + _bodyFile);
			setStatusCode(500);
			setState(ERROR);
			return false;
		}
		_state = CGI;
	}
	catch (const std::exception& e) 
	{

		LOG_INFO(e.what());
		setStatusCode(500);
		setState(ERROR);
		return false;
	}
	return true;
}

void HTTPRequest::writeBodyToFile(std::string& data) 
{
	if (!_body.is_open()) 
	{
		setStatusCode(500);
		setState(ERROR);
		return;
	}

	_body.write(data.c_str(), data.size());
	if (!_body.good()) 
	{
		setStatusCode(500);
		setState(ERROR);
		_body.close();
		return;
	}

	_length += data.size();

	data.clear();

	if (_length >= _contentLength)
	{
		_body.close();
		setState(FINISH);
	}
}

std::string HTTPRequest:: getSocketIp(int fd)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	if (getsockname(fd, (struct sockaddr*)&addr, &len) == -1)
		throw std::runtime_error("getsockname failed");

	return inet_ntoa(addr.sin_addr);
}

const ServerConfig& HTTPRequest::findServerByHost(const std::string& value)
{
	std::string host;
	uint16_t port = 0;

	size_t pos = value.find(':');
	if (pos != std::string::npos)
	{
		host = value.substr(0, pos);
		port = static_cast<uint16_t>(std::atoi(value.substr(pos + 1).c_str()));
	}
	else
		host = value;

	std::string ip = getSocketIp(_client_fd);

	std::map<std::string, std::vector<uint16_t> >::const_iterator it;

	for (size_t i = 0; i < _servers.size(); ++i)
	{
		if (_servers[i].getServerName() == host)
		{
			const std::map<std::string, std::vector<uint16_t> >& host_ports = _servers[i].getHostPort();
			it = host_ports.begin();
			for (it = host_ports.begin(); it != host_ports.end(); ++it)
			{
				if (it->first == ip)
				{
					const std::vector<uint16_t>& ports = it->second;
					for (size_t j = 0; j < ports.size(); ++j)
						if (port == 0 || ports[j] == port)
							return _servers[i];
				}
			}
		}
	}

	for (size_t i = 0; i < _servers.size(); ++i)
	{
		const std::map<std::string, std::vector<uint16_t> >& host_ports = _servers[i].getHostPort();
		it = host_ports.begin();
		for (it = host_ports.begin(); it != host_ports.end(); ++it)
		{
			if (it->first == ip)
			{
				const std::vector<uint16_t>& ports = it->second;
				for (size_t j = 0; j < ports.size(); ++j)
					if (port == 0 || ports[j] == port)
						return _servers[i];
			}
		}
	}
	return _servers[0];
}


void HTTPRequest::parseBody() 
{
	if (_state == BODY_INIT) 
	{
		if (!validateHostHeader() ||
			!validateAllowedMethods() ||
			!validateContentLength() ||
			!validateMultipartFormData() ||
			!validateCgi() ||
			!validateTransferEncoding())
			return;
	}
}


void HTTPRequest::parseChunkBody(std::string& data) 
{
	if (!_body.is_open()) 
	{
		_bodyFile = Utils::createTempFile("body_", _server.getClientBodyTmpPath());
		_body.open(_bodyFile.c_str(), std::ios::binary);
		if (!_body.is_open()) 
		{
			setStatusCode(500);
			setState(ERROR);
			return;
		}
		_chunkState = CHUNK_SIZE;
		_totalBodySize = 0;
	}

	while (!data.empty()) 
	{
		if (_chunkState == CHUNK_SIZE) 
		{
			if (!readChunkSize(data))
				return;
		}
		else if (_chunkState == CHUNK_DATA) 
		{
			if (!readChunkData(data))
				return;
		}
	}
}

bool HTTPRequest::readChunkSize(std::string& data) 
{
	size_t pos = data.find("\r\n");
	if (pos == std::string::npos)
		return false;

	std::string line = data.substr(0, pos);

	_chunkSize = Utils::parseHexChunk(line);
	if (_chunkSize == -1) 
	{
		setStatusCode(400);
		setState(ERROR);
		_body.close();
		return false;
	}
	data.erase(0, pos + 2);
	if (_chunkSize == 0) 
	{
		size_t pos = data.find("\r\n");
		if (pos == std::string::npos)
			return false;

		if (pos == 0)
		{
			data.erase(0, 2);
			_body.close();
			setStatusCode(200);
			setState(FINISH);
			return false;
		}
	}
	_chunkState = CHUNK_DATA;
	return true;
}

bool HTTPRequest::readChunkData(std::string& data) 
{
	if (data.size() < static_cast<size_t>(_chunkSize + 2))
		return false;

	if (data[_chunkSize] != '\r' || data[_chunkSize + 1] != '\n') 
	{
		setStatusCode(400);
		setState(ERROR);
		_body.close();
		return false;
	}

	_totalBodySize += _chunkSize;
	if (_totalBodySize > _server.getClientMaxBodySize()) 
	{
		setStatusCode(413);
		setState(ERROR);
		_body.close();
		return false;
	}
	if (getHeader("content-type").find("multipart/form-data") != std::string::npos)
	{
		std::string chunk_part = data.substr(0, _chunkSize);
		parseMultipartBody(chunk_part);
	}
	else 
		_body.write(data.c_str(), _chunkSize);

	data.erase(0, _chunkSize + 2);
	_chunkState = CHUNK_SIZE;
	return true;
}


void HTTPRequest::parseMultipartBody(std::string& data) 
{
	while (!data.empty()) 
	{
		
		if (_multipartState == PART_HEADER) 
		{
			if (!processPartHeader(data))
				return;
		}
		else if (_multipartState == PART_DATA) 
		{
			if (!processPartData(data))
				return;
		}
		else if (_multipartState == PART_BOUNDARY) 
		{
			if (!processPartBoundary(data))
				return;
		} 
	}
}

bool HTTPRequest::processPartHeader(std::string& data) 
{
	size_t end = data.find("\r\n\r\n");
	if (end == std::string::npos) 
		return false;

	std::string headers = data.substr(0, end);
	data.erase(0, end + 4);
	size_t filename_pos = headers.find("filename=\"");
	if (filename_pos == std::string::npos) 
		return false;

	filename_pos += 10;
	size_t quote_end = headers.find("\"", filename_pos);
	if (quote_end == std::string::npos) 
		return false;
	std::string filename = headers.substr(filename_pos, quote_end - filename_pos);

	std::string filePath = Utils::createUploadFile(filename, _location.getUploadPath());
	_uploadFile.open(filePath.c_str(), std::ios::binary);
	if (!_uploadFile.is_open()) 
	{
		setStatusCode(500);
		setState(ERROR);
		return false;
	}
	_multipartState = PART_DATA;
	return true;
}

bool HTTPRequest::processPartData(std::string& data) 
{
	size_t boundary_pos = data.find(_boundary);
	if (boundary_pos == std::string::npos) 
	{
		if (getHeader("transfer-encoding") == "chunked")
		{
			_uploadFile.write(data.c_str(), data.size());
			data.erase(0, data.size());
			_state = CHUNKED;
			return false;
		}

		if (data.size() <= _boundary.size() + 4)
			return false;
		size_t write_len = data.size() - (_boundary.size() + 4);
		_uploadFile.write(data.c_str(), write_len);
		data.erase(0, write_len);
		return false;
	}
	size_t data_end = boundary_pos;
	if (boundary_pos >= 2 && data.substr(boundary_pos - 2, 2) == "\r\n")
		data_end -= 2;
	if (data_end > 0) 
		_uploadFile.write(data.c_str(), data_end);

	_uploadFile.close();
	data.erase(0, boundary_pos);
	_multipartState = PART_BOUNDARY;
	return true;
}

bool HTTPRequest::processPartBoundary(std::string& data) 
{
	if (data.size() < _boundary.length() + 4) 
		return false;

	data.erase(0, _boundary.length());

	if (data.substr(0, 2) == "--") 
	{
		data.erase(0, 2);
		_multipartState = PART_END;
		setState(FINISH);
		setStatusCode(201);
		return false;
	}
	else if (data.substr(0, 2) == "\r\n") 
	{
		data.erase(0, 2);
		_multipartState = PART_HEADER;
		return true;
	}
	return false;
}

bool HTTPRequest::validateHostHeader() 
{
	std::string host = getHeader("host");

	if (host.empty()) 
	{
		setStatusCode(400);
		setState(ERROR);
		return false;
	}

	_server = findServerByHost(host);
	_location = _server.findLocation(_path);
	_resource = _location.getResource(_path);
	if (_resource.empty())
	{
		setState(ERROR);
		setStatusCode(400);
	}

	LOG_INFO(_method + " " + _uri + " " + _protocol);
	std::string hostname;
	std::string port;

	size_t colonPos = host.find(':');
	if (colonPos != std::string::npos) 
	{
		hostname = host.substr(0, colonPos);
		port = host.substr(colonPos + 1);

		for (size_t i = 0; i < port.size(); ++i) 
		{
			if (!isdigit(port[i])) 
			{
				setStatusCode(400);
				setState(ERROR);
				return false;
			}
		}
	} 
	else 
		hostname = host;

	if (hostname.empty()) 
	{
		setStatusCode(400);
		setState(ERROR);
		return false;
	}

	setState(FINISH);
	return true;
}

bool HTTPRequest::validateContentLength() 
{
	std::string cl = getHeader("content-length");
	if (cl.empty())
		return true;

	for (size_t i = 0; i < cl.size(); ++i)
	{
		if (!std::isdigit(cl[i])) 
		{
			setStatusCode(400);
			setState(ERROR);
			return false;
		}
	}
	_contentLength = Utils::stringToSizeT(cl);
	if (_contentLength > _server.getClientMaxBodySize()) 
	{
		setStatusCode(413);
		setState(ERROR);
		return false;
	}
	setState(FINISH);
	return true;
}

bool HTTPRequest::validateTransferEncoding() 
{
	std::string te = getHeader("transfer-encoding");
	std::string cl = getHeader("content-length");
	if (!te.empty()) 
	{
		if (te != "chunked") 
		{
			setStatusCode(501);
			setState(ERROR);
			return false;
		}
		if (!cl.empty()) 
		{
			setStatusCode(400);
			setState(ERROR);
			return false;
		}
		_isChunked = true;
		_state = CHUNKED;
		_chunkState = CHUNK_SIZE;
	}
	return true;
}

bool HTTPRequest::validateMultipartFormData() 
{
	if (_location.hasCgi())
		return true;

	std::string ct = getHeader("content-type");
	if (ct.find("multipart/form-data") == std::string::npos)
		return true;
	size_t pos = ct.find("boundary=");

	if (pos == std::string::npos) 
	{
		setStatusCode(400);
		setState(ERROR);
		return false;
	}
	_boundary += ct.substr(pos + 9);
	_multipartState = PART_HEADER;
	_state = MULTIPART;
	return true;
}

bool HTTPRequest::validateAllowedMethods() 
{
	if (_location.isMethodAllowed(_method))
		return true;
	setState(ERROR);
	setStatusCode(405);
	return false;
}

void HTTPRequest::clear() 
{
	if (_uploadFile.is_open())
		_uploadFile.close();
	if (_body.is_open())
	{
		_body.close();
		std::remove(_bodyFile.c_str());
	}

	_statusCode = 200;
	_state = INIT;
	_parsePosition = 0;
	_contentLength = 0;
	_method = "GET";
	_uri = "/";
	_path = "";
	_query = "";
	_protocol = "HTTP/1.1";
	_bodyBuffer = "";
	_headers.clear();
	_contentType = "";
	_boundary = "";
	_isChunked = false;
	_keepAlive = true;
	_chunkSize = -1;
	_boundary = "--";
	_multipartState = PART_HEADER;
	_chunkState = CHUNK_SIZE;
	_client_fd = -1;

}
