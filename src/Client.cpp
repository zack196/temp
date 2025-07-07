#include "../include/Client.hpp"
#include "../include/HTTPRequest.hpp"
#include <cwchar>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <wait.h>
#include <iostream>

Client::Client(int fd, std::vector<ServerConfig>& servers) : _fd(fd), _request(servers), _response(&_request), _lastActivity(time(NULL))
{
}

Client::~Client()
{
	if (_fd >= 0)
		close(_fd);
}

void Client::reset()
{
	_request.clear();
	_response.clear();
	_readBuffer.clear();
	updateActivity();
}

void Client::readRequest()
{
	char buffer[BUFFER_SIZE];
	std::memset(buffer, 0, sizeof(buffer));

	ssize_t bytesRead = recv(_fd, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead > 0) 
	{
		buffer[bytesRead] = '\0';
		_readBuffer.append(buffer, bytesRead);
		_request.parseRequest(_readBuffer);
	}
	else if (bytesRead == 0) 
		throw std::runtime_error("Client disconnected");
	else
		throw std::runtime_error("recv() failed unexpectedly");
}

void Client::sendResponse()
{

	char buffer[BUFFER_SIZE];
	std::memset(buffer, 0, sizeof(buffer));
	ssize_t bytesToSend = _response.getResponseChunk(buffer);

	if (bytesToSend > 0) 
	{
		ssize_t bytesSent = send(_fd, buffer, bytesToSend, 0);
		if (bytesSent <= 0)
			throw std::runtime_error("send() failed");
	}
	else if (bytesToSend == 0) 
		return;
	else
		throw std::runtime_error("Error generating response");
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

bool Client::shouldKeepAlive() const
{
	return _response.shouldKeepAlive();
}
