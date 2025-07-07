#include "../include/HTTPResponse.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/Utils.hpp"
#include "../include/Logger.hpp"
#include <algorithm>
#include <csetjmp>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <errno.h>
#include <strings.h>
#include <sys/wait.h>
#include <unistd.h>


HTTPResponse::HTTPResponse(HTTPRequest* request)
	: _request(request),
	_protocol("HTTP/1.1"),
	_statusCode(200),
	_statusMessage("OK"),
	_body(""),
	_header(""),
	_filePath(""),
	_fileSize(0),
	_cgiRuning(false),
	_bytesSent(0),
	_headerSent(false),
	_isComplete(false),
	_isReady(false)
{
}

HTTPResponse::~HTTPResponse()
{
	if (_fileStream.is_open())
		_fileStream.close();

	if (_cgiOutput.is_open())
		_cgiOutput.close();

}

bool HTTPResponse::isComplete()
{
	return (_isComplete);
}

bool HTTPResponse::isReady()
{
	if (_request->hasCgi())
		if (isCgiRuning())
			return false;

	return (_isReady);
}
void HTTPResponse::clear() 
{
	_protocol      = "HTTP/1.1";
	_statusCode    = 200;
	_statusMessage = "OK";
	_headers.clear();
	_body.clear();
	_header.clear();
	_filePath.clear();
	_fileSize      = 0;
	_bytesSent     = 0;
	_headerSent    = false;
	_cgiRuning    = false;
	_isComplete    = false;
	_isReady     =  false;

	_hasCgiOutput = false;
	_cgiHeaderComplete = false;
	_cgiHandler.cleanup();
}

ssize_t HTTPResponse::getResponseChunk(char* buffer)
{
	if (!buffer)
		return -1;

	size_t totalResponseSize = _header.size() + getContentLength();

	if (_bytesSent >= totalResponseSize)
	{
		if (_fileStream.is_open())
			_fileStream.close();
		if (_cgiOutput.is_open())
		{
			_cgiOutput.close();
			std::remove(_cgiFile.c_str());
		}

		return (_isComplete = true, 0);
	}

	else if (!_headerSent || _bytesSent < _header.size())
		return getHeaderChunk(buffer);
	else if (!_body.empty() || !_filePath.empty())
		return getContentChunk(buffer);

	return 1;

}

ssize_t HTTPResponse::getHeaderChunk(char* buffer)
{
	
	std::memcpy(buffer, _header.c_str(), _header.size());
	_bytesSent += _header.size();
	_headerSent = true;

	return _header.size();
}



ssize_t HTTPResponse::getContentChunk(char* buffer)
{
	
	if (!_body.empty())
	{
		std::memcpy(buffer, _body.c_str(), _body.size());
		_bytesSent += _body.size();
		return _body.size();
	}

	else if (!_filePath.empty())
	{
		if (!_fileStream.is_open())
		{
			_fileStream.open(_filePath.c_str(), std::ios::binary);
			if (!_fileStream.is_open())
				return -1;
		}

		_fileStream.read(buffer, BUFFER_SIZE);
		ssize_t bytesRead = _fileStream.gcount();

		if (bytesRead > 0)
		{
			_bytesSent += bytesRead;
			return bytesRead;
		}

		_fileStream.close();
		return 0;
	}

	return 0;
}

bool HTTPResponse::isCgiRuning()
{
	if (_cgiHandler.getPid() <= 0)
		return false;

	if (_cgiHandler.hasTimedOut())
	{
		LOG_DEBUG("CGI process timed out");
		_cgiHandler.killProcess();
		buildErrorResponse(504);
		return false;
	}
	int status;
	if (_cgiHandler.isRunning(status))
		return true;
	else
	{
		if (WIFEXITED(status))
		{
			int exitCode = WEXITSTATUS(status);
			if (exitCode != 0)
			{
				LOG_DEBUG("CGI exited with error code: " + Utils::toString(exitCode));
				buildErrorResponse(502);
				return false;
			}
		}
		buildCgiResponse();
		return false;
	}
}


CGIHandler& HTTPResponse::getCgiHandler()
{
	return _cgiHandler;
}

void HTTPResponse::startCgi()
{
	try
	{
		_cgiHandler.init(_request);
		_cgiHandler.start();
	}
	catch (const std::runtime_error& e)
	{
		LOG_ERROR(e.what());
		buildErrorResponse(403);
	}
	catch (const std::invalid_argument& e)
	{
		LOG_ERROR(e.what());
		buildErrorResponse(404);
	}
}

void HTTPResponse::readCgiFileAndParse(const std::string& filepath)
{
	std::ifstream file(filepath.c_str(), std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("Can't open CGI output file: " + filepath);
	std::string buffer;
	while (!file.eof())
	{
		buffer.resize(BUFFER_SIZE);
		file.read(&buffer[0], BUFFER_SIZE);
		size_t bytesRead = file.gcount();

		if (bytesRead > 0)
			parseCgiResponse(buffer.substr(0, bytesRead));
	}

	file.close();
}


void HTTPResponse::buildCgiResponse()
{
	try
	{
		readCgiFileAndParse(_cgiHandler.getOutputFile());
		setProtocol(_request->getProtocol());
		setStatusMessage(Utils::getMessage(_statusCode));

		if (_headers.find("Server") == _headers.end())
			setHeader("Server", "1337webserver");
		if (_headers.find("Date") == _headers.end())
			setHeader("Date", Utils::getCurrentDate());
		if (_headers.find("Content-Type") == _headers.end())
			setHeader("Content-Type", "text/html");
		if (_headers.find("Connection") == _headers.end())
			setHeader("Connection", shouldKeepAlive() ? "keep-alive" : "close");
		setBodyResponse(_cgiFile);
		setHeader("Content-Length", Utils::toString(getContentLength()));
		buildHeader();
		_isReady = true;

	}
	catch (const std::exception& e) 
	{
		buildErrorResponse(500);
	}
}





void HTTPResponse::setProtocol(const std::string& protocol) 
{
	_protocol = protocol;
}

void HTTPResponse::setStatusCode(int code) 
{
	_statusCode = code;
}

void HTTPResponse::setStatusMessage(const std::string& message) 
{
	_statusMessage = message;
}

void HTTPResponse::setHeader(const std::string& key, const std::string& value) 
{
	_headers[key] = value;
}

void HTTPResponse::setBodyResponse(const std::string& body) 
{
	struct stat file_stat;
	if (stat(body.c_str(), &file_stat) == 0)
	{
		_filePath = body;
		_fileSize = file_stat.st_size;
	}
	else 
		_body += body;
}


std::string HTTPResponse::getHeader() const 
{
	return _header;
}

std::string HTTPResponse::getBody() const 
{
	return _body;
}

std::string HTTPResponse::getFilePath() const 
{
	return _filePath;
}

size_t HTTPResponse::getContentLength() const 
{
	return !_body.empty() ? _body.size() : _fileSize;
}

size_t HTTPResponse::getFileSize() const 
{
	return _fileSize;
}

bool HTTPResponse::shouldKeepAlive() const 
{
	int reqStatus = _request->getStatusCode();
	std::string connection = Utils::trim(_request->getHeader("connection"));

	if (connection == "close")
		return false;
	else if (connection == "keep-alive")
		return true;
	else if (reqStatus == 400 || reqStatus == 408 || reqStatus == 414 || reqStatus == 431 || reqStatus == 501 || reqStatus == 505)
		return false;
	return true;
}

void HTTPResponse::buildHeader()
{
	_header = _protocol + " " + Utils::toString(_statusCode) + " " + _statusMessage + "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
		_header += it->first + ": " + it->second + "\r\n";

	_header += "\r\n";
}


void HTTPResponse::buildResponse() 
{
	std::string resource = _request->getResource();

	if (_request->getState() == HTTPRequest::ERROR)
		buildErrorResponse(_request->getStatusCode());

	else if (_request->getLocation().isAutoIndexOn() && Utils::isDirectory(resource)) 
		handleAutoIndex();

	else if (_request->getLocation().hasRedirection())
		handleRedirect();
	
	else if (_request->getMethod() == "GET")
		handleGet();
	else if (_request->getMethod() == "POST")
		handlePost();
	else if (_request->getMethod() == "DELETE")
		handleDelete();
	else
		buildErrorResponse(405);
}

void HTTPResponse::buildErrorResponse(int statusCode) 
{
	std::string defaultPage = _request->getServer().getErrorPage(statusCode);
	setBodyResponse(defaultPage);
	setProtocol(_request->getProtocol());
	setStatusCode(statusCode);
	setStatusMessage(Utils::getMessage(statusCode));
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Content-Type", "text/html");

	setHeader("Content-Length", Utils::toString(getContentLength()));
	setHeader("Connection", shouldKeepAlive() ? "keep-alive" : "close");
	buildHeader();
	_isReady = true;
}

void HTTPResponse::buildSuccessResponse(const std::string& fullPath) 
{
	setProtocol(_request->getProtocol());
	setStatusCode(_request->getStatusCode());
	setStatusMessage(Utils::getMessage(_request->getStatusCode()));
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldKeepAlive() ? "keep-alive" : "close");
	setHeader("Content-Type", Utils::getMimeType(fullPath));
	setBodyResponse(fullPath);
	setHeader("Content-Length", Utils::toString(getContentLength()));
	buildHeader();
	_isReady = true;
}

void HTTPResponse::handleGet() 
{
	if (_request->hasCgi()) 
		startCgi();

	else 
	{
		std::string resource = _request->getResource();

		if (!Utils::isFileExists(resource)) 
			return(buildErrorResponse(404));

		else if (!Utils::isFileReadble(resource)) 
			return(buildErrorResponse(403));
		else
			buildSuccessResponse(resource);
	}

}

void HTTPResponse::handlePost() 
{

	if (_request->hasCgi()) 
		startCgi();

	else 
	{
		std::string resource = _request->getResource();
		if (resource.empty() || !Utils::isFileExists(resource)) 
			return(buildErrorResponse(404));

		setProtocol(_request->getProtocol());
		setStatusCode(_request->getStatusCode());
		setStatusMessage("Created");
		setHeader("Content-Type", "text/html");
		setHeader("Server", "1337webserv/1.0");
		setHeader("Date", Utils::getCurrentDate());
		setHeader("Connection", shouldKeepAlive() ? "keep-alive" : "close");
		setHeader("Content-Length", Utils::toString(getContentLength()));
		buildHeader();
		_isReady = true;
	}

}

void HTTPResponse::handleDelete()
{
	std::string resource = _request->getResource();
	if (resource.empty() || !Utils::isFileExists(resource)) 
		return(buildErrorResponse(404));

	if (Utils::isDirectory(resource) || !Utils::isFileWritable(resource)) 
		return (buildErrorResponse(403));

	if (std::remove(resource.c_str()) != 0) 
	{
		LOG_ERROR("Failed to delete file " + resource + ": " + std::string(strerror(errno)));
		return (buildErrorResponse(500));
	}
	setProtocol(_request->getProtocol());
	setStatusCode(200);
	setStatusMessage("OK");
	setHeader("Content-Type", "text/plain");
	setHeader("Server", "1337webserv/1.0");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldKeepAlive() ? "keep-alive" : "close");
	setBodyResponse("Resource deleted successfully\n");
	setHeader("Content-Length", Utils::toString(getContentLength()));
	buildHeader();
	_isReady = true;
}

void HTTPResponse::handleRedirect()
{
	const LocationConfig location = _request->getLocation();

	int code = location.getRedirectCode();
	std::string reason = Utils::getMessage(code);
	std::string target = location.getRedirectPath();

	setProtocol(_request->getProtocol());
	setStatusCode(code);
	setStatusMessage(reason);

	setHeader("Content-Type", "text/html");
	setHeader("Location", target);
	setHeader("Server", "1337webserv/1.0");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldKeepAlive() ? "keep-alive" : "close");

	setBodyResponse("<html>\n<head><title>");
	setBodyResponse(Utils::toString(code));
	setBodyResponse(" ");
	setBodyResponse(reason);
	setBodyResponse("</title></head>\n<body>\n<center><h1>");
	setBodyResponse(Utils::toString(code));
	setBodyResponse(" ");
	setBodyResponse(reason);
	setBodyResponse("</h1></center>\n<hr><center>1337webserv/1.0</center>\n</body>\n</html>");

	setHeader("Content-Length", Utils::toString(getContentLength()));
	buildHeader();
	_isReady = true;
}


void HTTPResponse::handleAutoIndex() 
{
	std::string directory_path = _request->getResource();
	if (!Utils::isDirectory(directory_path)) 
		return(buildErrorResponse(404));

	std::vector<std::string> entries = Utils::listDirectory(directory_path);
	std::string req_path = _request->getPath();
	if (req_path.empty() || req_path[req_path.size()-1] != '/')
		req_path += '/';
	setBodyResponse("<!DOCTYPE html><html><head>");
	setBodyResponse("<title>Index of ");
	setBodyResponse(req_path);
	setBodyResponse("</title></head><body><h1>Index of ");
	setBodyResponse(req_path);
	setBodyResponse("</h1><hr><pre>");
	if (req_path != "/")
		setBodyResponse("<a href=\"../\">../</a>\n");
	for (size_t i = 0; i < entries.size(); ++i) 
	{
		if (entries[i] == "..")
			continue;

		std::string full_path = directory_path + "/" + entries[i];
		bool is_dir = Utils::isDirectory(full_path);
		std::string link_name = entries[i] + (is_dir ? "/" : "");
		setBodyResponse("<a href=\"" + req_path + link_name + "\">" + link_name + "</a>\n");
	}

	setBodyResponse("</pre><hr></body></html>");
	setProtocol(_request->getProtocol());
	setStatusCode(200);
	setStatusMessage("OK");
	setHeader("Content-Type", "text/html");
	setHeader("Content-Length", Utils::toString(getContentLength()));
	buildHeader();
	_isReady = true;
}

void HTTPResponse::parseCgiResponse(const std::string& chunk)
{

	static std::string headerBuffer;

	if (_cgiHeaderComplete) 
		return (writeToFile(chunk));

	headerBuffer += chunk;
	size_t headerEnd = headerBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		headerEnd = headerBuffer.find("\n\n");
	if (headerEnd != std::string::npos) 
	{
		std::string headers = headerBuffer.substr(0, headerEnd);
		_cgiHeaderComplete = parseHeaders(headers);
		size_t bodyStart = headerEnd + (headerBuffer.find("\r\n\r\n") != std::string::npos ? 4 : 2);
		std::string body = headerBuffer.substr(bodyStart);
		if (!body.empty())
			writeToFile(body);

		headerBuffer.clear();

	} 
	else if (headerBuffer.size() >= 8192) 
		buildErrorResponse(502);
	else 
	{
		writeToFile(headerBuffer);
		headerBuffer.clear();
		_cgiHeaderComplete = true;
	}

}

bool HTTPResponse::parseHeaders(const std::string& headers)
{
	std::istringstream headerStream(headers);
	std::string line;
	bool hasValidHeaders = false;

	while (std::getline(headerStream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		if (line.empty())
			continue;

		std::string::size_type colonPos = line.find(':');
		if (colonPos == std::string::npos)
			continue;

		std::string key = line.substr(0, colonPos);
		std::string value = line.substr(colonPos + 1);

		std::string::size_type start = value.find_first_not_of(" \t");
		if (start != std::string::npos)
			value = value.substr(start);

		std::string::size_type end = value.find_last_not_of(" \t");
		if (end != std::string::npos)
			value = value.substr(0, end + 1);

		if (key == "Status")
		{
			hasValidHeaders = true;

			std::istringstream statusStream(value);
			int code;
			statusStream >> code;

			std::string reason;
			std::getline(statusStream, reason);

			start = reason.find_first_not_of(" \t");
			if (start != std::string::npos)
				reason = reason.substr(start);
			setStatusCode(code);
		}
		else
		{
			hasValidHeaders = true;
			setHeader(key, value);
		}
	}
	return hasValidHeaders;
}

void HTTPResponse::writeToFile(const std::string& data)
{
	if (data.empty())
		return;
	if (!_cgiOutput.is_open())
	{
		_cgiFile = Utils::createTempFile("cgi_response_", _request->getServer().getClientBodyTmpPath());
		_cgiOutput.open(_cgiFile.c_str(), std::ios::out | std::ios::binary);

		if (!_cgiOutput.is_open())
			throw std::runtime_error("Failed to open CGI temp file: " + _cgiFile);
	}

	_cgiOutput.write(data.c_str(), data.size());
	_cgiOutput.flush();

	if (!_cgiOutput)
		throw std::runtime_error("Failed to write data to CGI temp file: " + _cgiFile);
}
