#include "../include/webserver.hpp"
#include "../include/ServerManager.hpp"
#include "../include/LocationConfig.hpp"
#include "../include/Utils.hpp"

// ServerManager.cpp
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>


ServerManager::ServerManager() 
{
	LOG_INFO("ServerManager initialized");
}

ServerManager::~ServerManager() 
{
	cleanup();
	LOG_INFO("ServerManager shutdown complete");
}

void ServerManager::setupServers(const std::vector<ServerConfig>& servers) 
{
	_servers = servers;
	LOG_INFO("Initializing " + Utils::toString(_servers.size()) + " servers");

	for (size_t i = 0; i < _servers.size(); ++i) 
	{
		_servers[i].setupServer();
		const std::vector<int>& fds = _servers[i].getFds();

		for (size_t j = 0; j < fds.size(); ++j) 
		{
			int server_fd = fds[j];
			setNonBlocking(server_fd);
			_epollManager.addFd(server_fd, EPOLLIN | EPOLLET);
			_serverMap[server_fd] = &_servers[i];
		}
	}
}

void ServerManager::run() 
{
	std::vector<struct epoll_event> events(MAX_EVENTS);
	time_t last_timeout_check = time(NULL);
	LOG_INFO("Server started and running");

	while (true) 
	{
		int num_events = _epollManager.wait(events, 1000);
		time_t now = time(NULL);

		if (now - last_timeout_check > TIMEOUT_CHECK_INTERVAL) 
		{
			checkTimeouts();
			last_timeout_check = now;
		}

		for (int i = 0; i < num_events; ++i) 
			handleSingleEvent(events[i]);
	}
}

void ServerManager::cleanup() 
{
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) 
		closeConnection(it->first);

	for (size_t i = 0; i < _servers.size(); ++i) 
	{
		const std::vector<int>& fds = _servers[i].getFds();
		for (size_t j = 0; j < fds.size(); ++j) 
		{
			close(fds[j]);
			LOG_DEBUG("Closed server socket (fd: " + Utils::toString(fds[j]) + ")");
		}
	}
}

void ServerManager::handleSingleEvent(struct epoll_event& event) 
{
	int fd = event.data.fd;

	if (_serverMap.find(fd) != _serverMap.end()) 
		handleNewConnection(fd);
	else if (_clients.find(fd) != _clients.end()) 
		handleClientEvent(fd, event.events);
	else 
	{
		LOG_ERROR("Unknown file descriptor in epoll event: " + Utils::toString(fd));
		close(fd);
	}
}

void ServerManager::handleNewConnection(int server_fd) 
{
	ServerConfig* server = _serverMap[server_fd];

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	memset(&client_addr, 0, client_len);

	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

	if (client_fd == -1) 
		return(LOG_ERROR("Accept failed on fd " + Utils::toString(server_fd) + ": " + strerror(errno)));
	try 
	{
		setNonBlocking(client_fd);
		_epollManager.addFd(client_fd, EPOLLIN | EPOLLET | EPOLLRDHUP);
		_clients[client_fd] = new Client(client_fd, server);
	}
	catch (const std::exception& e) 
	{
		LOG_ERROR("Failed to setup new connection: " + std::string(e.what()));
		close(client_fd);
	}
}

void ServerManager::handleClientEvent(int client_fd, uint32_t events) 
{
	Client* client = _clients[client_fd];
	client->updateActivity();

	try 
	{
		if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) 
			throw std::runtime_error("Socket error or hangup");

		if (events & EPOLLIN) 
		{
			client->handleRequest();
			if (client->getRequest()->getState() == HTTPRequest::FINISH) 
				_epollManager.modifyFd(client_fd, EPOLLOUT | EPOLLET | EPOLLRDHUP);
		}
		if (events & EPOLLOUT) 
		{
			if (client->getRequest() && client->getRequest()->getState() == HTTPRequest::FINISH)
				client->handleResponse();
			if (client->getResponse()->getState() == HTTPResponse::FINISH) 
			{
				_epollManager.modifyFd(client_fd, EPOLLIN | EPOLLET | EPOLLRDHUP);
				client->reset();
			}
		}
	}
	catch (const std::exception& e) 
	{
		LOG_DEBUG("Closing connection due to error (fd: " + Utils::toString(client_fd) + "): " + e.what());
		closeConnection(client_fd);
	}
}

void ServerManager::closeConnection(int client_fd) 
{
	try
	{
		_epollManager.removeFd(client_fd);
		close(client_fd);
		delete _clients[client_fd];
		_clients.erase(client_fd);
		LOG_DEBUG("Closed connection (fd: " + Utils::toString(client_fd) + ")");
	}
	catch (const std::exception& e) 
	{
		LOG_ERROR("Error closing connection (fd: " + Utils::toString(client_fd) + "): " + e.what());
	}
}

void ServerManager::checkTimeouts() {
	time_t now = time(NULL);
	std::vector<int> toClose;

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) 
	{
		if (now - it->second->getLastActivity() > CONNECTION_TIMEOUT) 
			toClose.push_back(it->first);
	}

	for (size_t i = 0; i < toClose.size(); ++i) 
	{
		LOG_INFO("Closing idle connection (fd: " + Utils::toString(toClose[i]) + ")");
		closeConnection(toClose[i]);
	}
}

void ServerManager::setNonBlocking(int fd) 
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) 
		throw std::runtime_error("fcntl failed: " + std::string(strerror(errno)));
}

