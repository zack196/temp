/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/10 17:36:21 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/19 20:54:43 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Client.hpp"
#include "../include/Logger.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/HTTPResponse.hpp"
#include "../include/Utils.hpp"
#include "../include/webserver.hpp"


Client::Client(int fd_client, ServerConfig* server) : _fd(fd_client),_server(server)
	, _bytesSent(0),_request(new HTTPRequest(this)),_response(new HTTPResponse(this))
{
    _lastActivity = time(NULL);
}

Client::~Client() 
{
	delete _request;
	delete _response;
}

int Client::getFd() const 
{
	return _fd;
}

time_t Client::getLastActivity()const
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

size_t& Client::getBytesSent() 
{
	return _bytesSent;
}

void Client::parseRequest(const char* data, size_t length) 
{
		
	_readBuffer.append(data, length);
	if (this->_request->getState() == HTTPRequest::FINISH)
		return (LOG_DEBUG("[handleRequest] Request already finished"));

	this->_request->parse(_readBuffer);
}

void Client::handleResponse() 
{	
	if (_response->buildResponse() == -1)
		return (LOG_DEBUG("[handleRequest] Request already finished"));
	ssize_t byteSend = send(_fd, _response->getResponse().c_str()
		, _response->getResponse().size(), 0);
	if (byteSend == -1)
		return (LOG_ERROR("Send failed (fd: "  ")"));
}

HTTPRequest* Client::getRequest() 
{
	return _request;
}

void Client::clearBuffers() 
{
	_readBuffer.clear();
	_writeBuffer.clear();
	_bytesSent = 0;
}
