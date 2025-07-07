#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <ctime>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "ServerConfig.hpp"

class Client
{
private:
	int                             _fd;
	HTTPRequest                     _request;
	HTTPResponse                    _response;
	std::string                     _readBuffer;
	time_t                          _lastActivity;

public:
	Client(int fd, std::vector<ServerConfig>& servers);
	~Client();

	void readRequest();
	void sendResponse();
	void reset();

	int getFd() const;
	time_t getLastActivity() const;
	HTTPRequest* getRequest();
	HTTPResponse* getResponse();
	bool shouldKeepAlive() const;

	void updateActivity();
};

#endif
