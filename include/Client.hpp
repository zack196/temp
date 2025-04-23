/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/10 16:06:48 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/18 23:46:23 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>
#include <ctime>
#include "ServerConfig.hpp"

# include <string>
class HTTPResponse;
class HTTPRequest;

class Client 
{
	private:
		int				_fd;
		ServerConfig*		_server;
		std::string		_readBuffer;
		std::string		_writeBuffer;
		size_t			_bytesSent;
		time_t 			_lastActivity;
		HTTPRequest		*_request;
		HTTPResponse		*_response;

	public:
		Client(int fd_client, ServerConfig* server);
		~Client();

		int					getFd() const;
		const std::string&			getWriteBuffer() const;
		std::string&				getWriteBuffer();
		const std::string&			getReadBuffer() const;
		size_t&					getBytesSent();
		HTTPRequest* getRequest();



		void parseRequest(const char* data, size_t length);
		void	handleResponse();
		void	clearBuffers();
		void	setServer(ServerConfig* server); 
		ServerConfig* getServer() const;
		time_t getLastActivity() const;
		void updateActivity();


};

#endif 
