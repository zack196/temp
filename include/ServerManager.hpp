#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <vector>
#include <map>
#include <string>
#include <ctime>
#include <cstring>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ServerConfig.hpp"
#include "Client.hpp"
#include "Epoll.hpp"

class ServerManager
{
private:
	Epoll                        _epoll;
	bool                                _running;
	std::vector<ServerConfig>           _servers;
	std::vector<int>                    _serverFds;
	std::map<int, Client*>              _clients;

public:
	ServerManager(const std::vector<ServerConfig>& servers);
	~ServerManager();

	bool init();
	void run();
	void stop();

private:
	int createAndBindSocket(const std::string& host, uint16_t port);

	void handleEvent(const epoll_event& event);
	void acceptClient(int fd);

	void checkTimeouts();
	void cleanupClient(int fd);
};

#endif
