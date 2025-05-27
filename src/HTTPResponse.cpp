#include "../include/HTTPResponse.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/Utils.hpp"
#include "../include/Logger.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <errno.h>
#include <ostream>
#include <strings.h>
#include <sys/wait.h>

HTTPResponse::HTTPResponse(HTTPRequest* request)
	: _request(request),
	_protocol("HTTP/1.1"),
	_statusCode(200),
	_statusMessage("OK"),
	_body(""),
	_header(""),
	_filePath(""),
	_fileSize(0),
	_state(INIT),
	_cgiHeaderComplete(false),
	_hasCgiOutput(false)
{
	if (!_request)
		throw std::runtime_error("Invalid HTTPRequest pointer in HTTPResponse constructor");
}

HTTPResponse::HTTPResponse(const HTTPResponse& rhs) 
{
	*this = rhs;
}

HTTPResponse& HTTPResponse::operator=(const HTTPResponse& rhs) 
{
	if (this != &rhs) 
	{
		_request       = rhs._request;
		_protocol      = rhs._protocol;
		_statusCode    = rhs._statusCode;
		_statusMessage = rhs._statusMessage;
		_headers       = rhs._headers;
		_body          = rhs._body;
		_header        = rhs._header;
		_filePath      = rhs._filePath;
		_fileSize      = rhs._fileSize;
		_state         = rhs._state;
	}
	return *this;
}

HTTPResponse::~HTTPResponse()
{
	if (_cgiStream.is_open())
		_cgiStream.close();
}

void HTTPResponse::clear() 
{
	_statusCode    = 200;
	_protocol      = "HTTP/1.1";
	_statusMessage = "OK";
	
	_setCookies.clear();

	_headers.clear();
	_body.clear();
	_header.clear();
	_filePath.clear();
	_fileSize      = 0;
	_state         = INIT;
	if (_cgiStream.is_open())
		_cgiStream.close();

	if (!_cgiTempFile.empty() && std::remove(_cgiTempFile.c_str()) == 0) 
		LOG_INFO("Removed temporary body file: " + _cgiTempFile);

	_hasCgiOutput = false;
	_cgiHeaderComplete = false;

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

void HTTPResponse::appendToBody(const std::string& bodyPart) 
{
	_body.append(bodyPart);
}

void HTTPResponse::setBodyResponse(const std::string& filePath) 
{
	_filePath = filePath;
	struct stat file_stat;
	if (stat(filePath.c_str(), &file_stat) == -1) 
	{
		LOG_ERROR("Failed to stat file " + filePath + ": " + std::string(strerror(errno)));
		throw std::runtime_error("Failed to stat file: " + filePath);
	}
	_fileSize = file_stat.st_size;
}

void HTTPResponse::setState(response_state state) 
{
	_state = state;
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

int HTTPResponse::getState() const 
{
	return _state;
}

bool HTTPResponse::shouldCloseConnection() const 
{
	int reqStatus = _request->getStatusCode();
	std::string connection = Utils::trim(_request->getHeader("connection"));

	if (connection == "close")
		return true;
	else if (connection == "keep-alive")
		return false;
	else if (reqStatus == 400 || reqStatus == 408 || reqStatus == 414 || reqStatus == 431 || reqStatus == 501 || reqStatus == 505)
		return true;
	return false;
}

void HTTPResponse::buildHeader()
{
	std::ostringstream oss;
	oss << _protocol << " " << _statusCode << " " << _statusMessage << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
		oss << it->first << ": " << it->second << "\r\n";
	
	for (size_t i = 0; i < _setCookies.size(); i++)
		oss << "Set-Cookie: " << _setCookies[i] << "\r\n";
	
	

	oss << "\r\n";
	_header = oss.str();
}


void HTTPResponse::buildResponse() 
{
	std::cout << "---------------------------------\n";
	_request->print();
	std::cout << "---------------------------------\n";
	
	if (_request->getState() == HTTPRequest::ERRORE)
		buildErrorResponse(_request->getStatusCode());
	else if (_request->hasCgi())
		handleCgi();
	else if (_request->getLocation() && _request->getLocation()->hasRedirection())
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
	std::string defaultPage = _request->getServer()->getErrorPage(statusCode);
	setProtocol(_request->getProtocol());
	setStatusCode(statusCode);
	setStatusMessage(Utils::getMessage(statusCode));
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Content-Type", "text/html");

	if (Utils::fileExists(defaultPage)) 
	{
		setBodyResponse(defaultPage);
		setHeader("Content-Length", Utils::toString(getFileSize()));
	}
	else 
	{
		appendToBody(defaultPage);
		setHeader("Content-Length", Utils::toString(_body.size()));
	}
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");
	buildHeader();
	setState(HEADER_SENT);
}

void HTTPResponse::buildSuccessResponse(const std::string& fullPath) 
{
	setProtocol(_request->getProtocol());
	setStatusCode(_request->getStatusCode());
	setStatusMessage(Utils::getMessage(_request->getStatusCode()));
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");
	setHeader("Content-Type", Utils::getMimeType(fullPath));
	setBodyResponse(fullPath);
	setHeader("Content-Length", Utils::toString(getContentLength()));
	buildHeader();
	setState(FINISH);
}

void HTTPResponse::handleGet() 
{
	
	std::string resource = _request->getResource();
	if (resource.empty()) 
		return(buildErrorResponse(404));

	else if (_request->getLocation() && _request->getLocation()->isAutoIndexOn() && Utils::isDirectory(resource)) 
		return(handleAutoIndex());

	else if (Utils::fileExists(resource)) 
		return(buildSuccessResponse(resource));
	else 
		return(buildErrorResponse(403));
}


void  HTTPResponse::handleCgi()
{
	setStatusCode(200);
	setStatusMessage("Created");
	setHeader("Content-Type", "text/html");
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");
	setHeader("Content-Length", Utils::toString(getContentLength()));
	buildHeader();
	setState(FINISH);
	
}

void HTTPResponse::handlePost() 
{
	std::string resource = _request->getResource();
	if (_request->getPath() == "/login")
	{
		std::cout << "in\n";
		handleLogin();
		return ;
	}
	if (resource.empty() || !Utils::fileExists(resource)) 
		return(buildErrorResponse(404));
	

	std::string body = "Resource created successfully\n";
	setProtocol(_request->getProtocol());
	setStatusCode(201);
	setStatusMessage("Created");
	setHeader("Content-Type", "text/plain");
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");
	appendToBody(body);
	setHeader("Content-Length", Utils::toString(_body.size()));
	buildHeader();
	setState(FINISH);

}

void HTTPResponse::handleDelete()
{
	std::string resource = _request->getResource();
	if (resource.empty() || !Utils::fileExists(resource)) 
		return(buildErrorResponse(404));

	if (Utils::isDirectory(resource)) 
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
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");
	appendToBody("Resource deleted successfully\n");
	setHeader("Content-Length", Utils::toString(_body.size()));
	buildHeader();
	setState(FINISH);
}

void HTTPResponse::handleRedirect() 
{
	const LocationConfig* location = _request->getLocation();
	if (!location || !location->hasRedirection()) 
	{
		return (buildErrorResponse(500));
	}
	int code = location->getRedirectCode();
	std::string reason = Utils::getMessage(code);
	std::string target = location->getRedirectPath();
	setProtocol(_request->getProtocol());
	setStatusCode(code);
	setStatusMessage(reason);
	setHeader("Content-Type", "text/html");
	setHeader("Location", target);
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");

	appendToBody("<html><head><title>");
	appendToBody(Utils::toString(code));
	appendToBody(" ");
	appendToBody(reason);
	appendToBody("</title></head><body><h1>");
	appendToBody(Utils::toString(code));
	appendToBody(" ");
	appendToBody(reason);
	appendToBody("</h1><hr><center>");
	appendToBody("1337webserver");
	appendToBody("</center></body></html>");

	setHeader("Content-Length", Utils::toString(_body.size()));
	buildHeader();
	setState(HEADER_SENT);
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
	// Build an HTML directory listing.
	appendToBody("<!DOCTYPE html><html><head>");
	appendToBody("<title>Index of ");
	appendToBody(req_path);
	appendToBody("</title></head><body><h1>Index of ");
	appendToBody(req_path);
	appendToBody("</h1><hr><pre>");
	if (req_path != "/")
		appendToBody("<a href=\"../\">../</a>\n");
	for (size_t i = 0; i < entries.size(); ++i) 
	{
		std::string full_path = directory_path + "/" + entries[i];
		bool is_dir = Utils::isDirectory(full_path);
		std::string link_name = entries[i] + (is_dir ? "/" : "");
		appendToBody("<a href=\"" + req_path + link_name + "\">" + link_name + "</a>\n");
	}
	appendToBody("</pre><hr></body></html>");
	setProtocol(_request->getProtocol());
	setStatusCode(200);
	setStatusMessage("OK");
	setHeader("Content-Type", "text/html");
	setHeader("Content-Length", Utils::toString(_body.size()));
	buildHeader();
}

void HTTPResponse::parseCgiOutput(const std::string& chunk)
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
		bool hasValidHeaders = parseHeaders(headers);

		size_t bodyStart = headerEnd + (headerBuffer.find("\r\n\r\n") != std::string::npos ? 4 : 2);
		std::string body = headerBuffer.substr(bodyStart);
		if (!body.empty())
			writeToFile(body);

		_cgiHeaderComplete = hasValidHeaders;
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
	setBodyResponse(_cgiTempFile);
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

		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos) 
		{
			hasValidHeaders = true;
			std::string key = line.substr(0, colonPos);
			std::string value = line.substr(colonPos + 1);

			value.erase(0, value.find_first_not_of(" \t"));
			value.erase(value.find_last_not_of(" \t") + 1);

			setHeader(key, value);
			LOG_DEBUG("Parsed CGI Header: " + key + ": " + value);
		}
	}

	return hasValidHeaders;
}

void HTTPResponse::writeToFile(const std::string& data)
{
	if (data.empty())
		return;

	if (!_hasCgiOutput)
		createCgiTempFile();

	_cgiStream.write(data.c_str(), data.size());
	_cgiStream.flush();

	if (!_cgiStream)
		throw std::runtime_error("Failed to write data to CGI temp file: " + _cgiTempFile);
}

void HTTPResponse::createCgiTempFile()
{
	if (_hasCgiOutput)
		return;

	_cgiTempFile = Utils::createTempFile("cgi_response_", _request->getServer()->getClientBodyTmpPath());

	if (_cgiStream.is_open())
		_cgiStream.close();

	_cgiStream.open(_cgiTempFile.c_str(), std::ios::out | std::ios::binary);
	if (!_cgiStream.is_open())
		throw std::runtime_error("Failed to open CGI temp file: " + _cgiTempFile);

	_hasCgiOutput = true;
	LOG_INFO("Created CGI temp file: " + _cgiTempFile);
}

void HTTPResponse::closeCgiTempFile()
{
	if (_cgiStream.is_open())
	{
		_cgiStream.flush(); // Ensure all data is written
		_cgiStream.close(); // Close the file
		LOG_INFO("Closed CGI temp file: " + _cgiTempFile);
	}
}

void HTTPResponse::addCookie(const std::string &name, const std::string &value, const Utils::CookieAttr &attr)
{
	_setCookies.push_back(name + "=" + value + Utils::buildCookieAttributes(attr));
}

void HTTPResponse::handleLogin()
{
    // (a) Parse x-www-form-urlencoded body -------------------------
        std::map<std::string,std::string> form =
            Utils::parseUrlEncoded(_request->getBody());

        std::string user = form["username"];
        std::string pass = form["password"];

        // (b) Verify credentials (placeholder) -------------------------
        if (user.empty() || pass != "secret")            // demo check
        {
            buildErrorResponse(401);                     // “Unauthorized”
            return;
        }

        // (c) Create session & Set-Cookie ------------------------------
        SessionManager* sm = _request->getServer()->getSessionManager();
        std::string sid   = sm->createSession(user);

        Utils::CookieAttr attr;      // Path=/  SameSite=Lax by default
        attr.httpOnly = true;
        addCookie("sid", sid, attr);

        // (d) Minimal success reply ------------------------------------
        appendToBody("Logged in as " + user + "\n");
        setProtocol(_request->getProtocol());
        setStatusCode(200);
        setStatusMessage("OK");
        setHeader("Content-Type", "text/plain");
        setHeader("Content-Length", Utils::toString(getBody().size()));
        setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");
        buildHeader();
        setState(FINISH);
}
