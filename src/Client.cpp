/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/10 17:36:21 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/24 03:36:50 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"
#include "../include/Logger.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/HTTPResponse.hpp"
#include "../include/Utils.hpp"
#include "../include/webserver.hpp"


// Client.cpp
Client::Client(int fd_client, ServerConfig* server) : _fd(fd_client),_server(server),_bytesSent(0),_request(new HTTPRequest(this)),_response(new HTTPResponse(this))
{
    _lastActivity = time(NULL);
}

Client::Client(const Client& other): _fd(other._fd),_server(other._server),_bytesSent(other._bytesSent),_lastActivity(other._lastActivity),_readBuffer(other._readBuffer),_writeBuffer(other._writeBuffer),_request(new HTTPRequest(*(other._request))),_response(new HTTPResponse(*(other._response)))
{
}

Client::~Client()
{
    delete _request;
    delete _response;
}

Client& Client::operator=(const Client& other)
{
    if (this != &other)
    {
        delete _request;
        delete _response;
        
        _fd = other._fd;
        _server = other._server;
        _bytesSent = other._bytesSent;
        _lastActivity = other._lastActivity;
        _readBuffer = other._readBuffer;
        _writeBuffer = other._writeBuffer;
        
        _request = new HTTPRequest(*(other._request));
        _response = new HTTPResponse(*(other._response));
    }
    return *this;
}

void	Client::handleRequest(void)
{
	LOG_DEBUG("[handleRequest] Handling request from client " + Utils::toString(this->_fd));
	
	char	buffer[BUFFER_SIZE + 1];
	int		bytesRead = 0;

	memset(buffer, 0, BUFFER_SIZE + 1);
	bytesRead  = recv(this->_fd, buffer, BUFFER_SIZE, 0);
	if (bytesRead > 0)
		buffer[bytesRead] = '\0';
	else if (bytesRead < 0)
		throw std::runtime_error("Error with recv function");
	else if (bytesRead == 0)
		throw std::runtime_error("Close connection");
	
	if (this->_request->getState() == HTTPRequest::FINISH)
		return (LOG_DEBUG("[handleRequest] Request already finished"));

	std::string str(buffer, bytesRead);
	this->_request->parse(str);
}


void Client::handleResponse()
{
	if (this->_response->buildResponse() == -1) // Reponse not ready
		return ;

	int bytesSent = -1;
	if (this->getFd() != -1)
		bytesSent = send(this->getFd(), this->_response->getResponse().c_str(), this->_response->getResponse().size(), 0);
	
	if (bytesSent < 0)
		throw std::runtime_error("Error with send function");
	else
		LOG_DEBUG("Sent " + Utils::toString(bytesSent) + " bytes to client " + Utils::toString(this->getFd()));

	if (this->getResponse()->getState() == HTTPResponse::FINISH)
	{
		if (this->_request->getState() != HTTPRequest::FINISH)
			throw std::runtime_error("Client Close connection");
		LOG_DEBUG("Response sent to client "  + Utils::toString( this->getFd()));
	}
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

ServerConfig* Client::getServer() const
{
    return _server;
}

const std::string& Client::getWriteBuffer() const
{
    return _writeBuffer;
}

std::string& Client::getWriteBuffer()
{
    return _writeBuffer;
}

const std::string& Client::getReadBuffer() const
{
    return _readBuffer;
}

void Client::reset(void)
{
	delete this->_request;
	this->_request = new HTTPRequest(this);
	delete this->_response;
	this->_response = new HTTPResponse(this);
}


size_t& Client::getBytesSent()
{
    return _bytesSent;
}
HTTPRequest* Client::getRequest()
{
    return _request;
}
HTTPResponse* Client::getResponse()
{
    return _response;
}
void Client::clearBuffers()
{
    _readBuffer.clear();
    _writeBuffer.clear();
    _bytesSent = 0;
}
