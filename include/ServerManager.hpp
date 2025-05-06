#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include "ServerConfig.hpp"
#include "EpollManager.hpp"
#include "Logger.hpp"
#include "../include/Client.hpp"

class ServerManager 
{
	private:
		std::vector<ServerConfig> _servers;
		std::map<int, ServerConfig*> _serverMap;
		std::map<int, Client*> _clients;
		EpollManager _epollManager;

		// Constants
		static const int MAX_EVENTS;
		static const int CONNECTION_TIMEOUT;
		static const int TIMEOUT_CHECK_INTERVAL;
		static const int EPOLL_TIMEOUT;

		// Private methods
		void cleanup();
		void handleSingleEvent(struct epoll_event& event);
		void handleNewConnection(int server_fd);
		void handleClientEvent(int client_fd, uint32_t events);
		void setNonBlocking(int fd);

	public:
		ServerManager();
		~ServerManager();

		void setupServers(const std::vector<ServerConfig>& servers);
		void run();
		void closeConnection(int client_fd);
		void checkTimeouts();
};


#endif


