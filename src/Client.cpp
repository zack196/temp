#include "../include/Client.hpp"
#include "../include/Logger.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/CGIHandler.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>


Client::Client(int fd, ServerConfig* server)
	: _fd(fd),
	_server(server),
	_request(this->getServer()),
	_response(this->getRequest()),
	_cgiHandler(NULL),
	_isCgi(false),
	_bytesSent(0),
	_headersSize(0),
	_lastActivity(time(NULL)),
	_cgiStartTime(0),
	_readBuffer("")
{
	if (fd < 0 || !server)
		throw std::runtime_error("Invalid client parameters");
}

Client::~Client()
{
	if (_cgiHandler)
	{
		_isCgi = false;
		_cgiHandler->cleanup(); // ensure the child process is terminated
		delete _cgiHandler;
		_cgiHandler = NULL;
	}

	if (_fileStream.is_open())
		_fileStream.close();

	if (_fd >= 0)
		close(_fd);

	LOG_DEBUG("Client destroyed, fd: " + Utils::toString(_fd));
}

std::string& Client::getReadBuffer()
{
	return _readBuffer;
}

void Client::setReadBuffer(const char* data, ssize_t bytesSet)
{
	_readBuffer.append(data, bytesSet);
}

ssize_t Client::getResponseChunk(char* buffer, size_t bufferSize)
{
	if (!buffer)
		return -1;

	const size_t totalResponseSize = _response.getHeader().size() + _response.getContentLength();

	if (_bytesSent >= totalResponseSize)
		return 0;

	else if (_bytesSent < _response.getHeader().size())
		return getHeaderResponse(buffer);

	else if (!_response.getBody().empty())
		return getStringBodyResponse(buffer);

	else if (!_response.getFilePath().empty())
		return getFileBodyResponse(buffer, bufferSize);

	return -1;
}

ssize_t Client::getHeaderResponse(char* buffer)
{
	const std::string& header = _response.getHeader();
	std::memcpy(buffer, header.c_str(), header.size());
	_bytesSent += header.size();
	_headersSize = header.size();
	return header.size();
}

ssize_t Client::getStringBodyResponse(char* buffer)
{
	const std::string& body = _response.getBody();
	std::memcpy(buffer, body.c_str(), body.size());
	_bytesSent += body.size();
	return body.size();
}

ssize_t Client::getFileBodyResponse(char* buffer, size_t bufferSize)
{
	const std::string& filePath = _response.getFilePath();

	if (!_fileStream.is_open())
	{
		_fileStream.open(filePath.c_str(), std::ios::binary);
		if (!_fileStream.is_open())
			return -1;
	}
	_fileStream.read(buffer, bufferSize);
	ssize_t bytesRead = _fileStream.gcount();

	if (bytesRead > 0)
	{
		_bytesSent += bytesRead;
		return bytesRead;
	}

	_fileStream.close();
	return 0;
}

int Client::getFd() const
{
	return _fd;
}

time_t Client::getLastActivity() const
{
	return _lastActivity;
}

void Client::updateActivity()
{
	_lastActivity = time(NULL);
}

HTTPRequest* Client::getRequest()
{
	return &_request;
}

HTTPResponse* Client::getResponse()
{
	return &_response;
}

ServerConfig* Client::getServer() const
{
	return _server;
}

bool Client::shouldKeepAlive() const
{
	return !_response.shouldCloseConnection();
}

bool Client::hasTimedOut(time_t currentTime, time_t timeout) const
{
	if (_isCgi) 
		return (currentTime - _cgiStartTime) > timeout;
	else 
		return (currentTime - _lastActivity) > timeout;
}

void Client::reset()
{
	_request.clear();
	_response.clear();
	_readBuffer.clear();
	_bytesSent = 0;
	_headersSize = 0;

	if (_fileStream.is_open())
		_fileStream.close();

	if (_cgiHandler)
	{
		_cgiHandler->cleanup();
		delete _cgiHandler;
		_cgiHandler = NULL;
	}
	_isCgi = false;

	updateActivity();
	// LOG_DEBUG("Client reset for keep-alive, fd: " + Utils::toString(_fd));
}

void Client::setCgiHandler(CGIHandler* cgiHandler)
{
	_cgiHandler = cgiHandler;
}

CGIHandler* Client::getCgiHandler() const
{
	return _cgiHandler;
}

void Client::setIsCgi(bool isCgi)
{
	_isCgi = isCgi;
}

bool Client::isCgi() const
{
	return _isCgi;
}
