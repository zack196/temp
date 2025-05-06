/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/10 16:06:48 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/23 18:38:23 by mregrag          ###   ########.fr       */
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

class Client {
	private:
		int _fd;
		ServerConfig* _server;
		size_t _bytesSent;
		time_t _lastActivity;
		std::string _readBuffer;
		std::string _writeBuffer;
		HTTPRequest* _request;
		HTTPResponse* _response;

	public:
		// Orthodox Canonical Form
		Client(int fd_client, ServerConfig* server);
		Client(const Client& other);
		~Client();
		Client& operator=(const Client& other);

		// Getters
		int getFd() const;
		void reset(); 
		time_t getLastActivity() const;
		ServerConfig* getServer() const;
		const std::string& getWriteBuffer() const;
		std::string& getWriteBuffer();
		const std::string& getReadBuffer() const;
		size_t& getBytesSent();
		HTTPRequest* getRequest();
		HTTPResponse* getResponse();

		// Methods
		void updateActivity();
		void handleRequest(void);
		void handleResponse();
		void clearBuffers();
};

#endif 
