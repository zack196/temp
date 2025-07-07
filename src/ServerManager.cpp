#include "../include/ServerManager.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Logger.hpp"
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <algorithm>

ServerManager::ServerManager(const std::vector<ServerConfig>& servers) : _epoll(EVENTS), _running(false), _servers(servers)
{
}

ServerManager::~ServerManager()
{
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) 
	{
		close(it->first);
		delete it->second;
	}
	_clients.clear();

	for (size_t i = 0; i < _serverFds.size(); ++i)
		close(_serverFds[i]);
	_serverFds.clear();
}

int ServerManager::createAndBindSocket(const std::string& host, uint16_t port)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		return -1;

	int opt = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		return (close(sockfd), -1);

	if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0)
		return (close(sockfd), -1);

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);

	if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) <= 0)
		return (close(sockfd), -1);

	if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0)
		return (close(sockfd), -1);


	if (listen(sockfd, 10) < 0)
		return (close(sockfd), -1);


	return sockfd;
}

bool ServerManager::init()
{
	std::map<std::string, std::map<uint16_t, int> > host_port_fd;
	for (size_t i = 0; i < _servers.size(); ++i)
	{
		try
		{
			const std::map<std::string, std::vector<uint16_t> >& host_ports = _servers[i].getHostPort();

			std::map<std::string, std::vector<uint16_t> >::const_iterator it;
			for (it = host_ports.begin(); it != host_ports.end(); ++it)
			{
				const std::string& host = it->first;
				const std::vector<uint16_t>& ports = it->second;

				for (size_t j = 0; j < ports.size(); ++j)
				{
					uint16_t port = ports[j];

					if (host_port_fd.count(host) && host_port_fd[host].count(port))
						continue;

					int fd = createAndBindSocket(host, port);
					if (fd < 0)
						throw std::runtime_error("Failed to bind socket for " + host + ":" + Utils::toString(port));

					if (!_epoll.add(fd, EPOLLIN))
					{
						close(fd);
						throw std::runtime_error("Failed to add socket to epoll for " + host + ":" + Utils::toString(port));
					}
					host_port_fd[host][port] = fd;
					_serverFds.push_back(fd);

					LOG_INFO("servser " + Utils::toString(i) + " Listening on " + host + ":" + Utils::toString(port) + " (fd " + Utils::toString(fd) + ")");
				}
			}
		}
		catch (const std::exception& e)
		{
			LOG_ERROR(e.what());
			return false;
		}
	}
	return true;
}


void ServerManager::run()
{
	_running = true;
	while (_running) 
	{
		try {
			int numEvents = _epoll.wait(1000);

			if (numEvents < 0) 
			{
				if (errno == EINTR)
					continue;
				else 
					throw std::runtime_error("epoll_wait failed");
			}
			for (int i = 0; i < numEvents; ++i) 
				handleEvent(_epoll.getEvent(i));
			checkTimeouts();
		}
		catch (const std::exception& e) 
		{
			LOG_ERROR("ERROR in main event loop " + std::string(e.what()));
		}
	}

}

void ServerManager::stop()
{
	_running = false;
}

void ServerManager::handleEvent(const epoll_event& event)
{
	int fd = event.data.fd;
	uint32_t events = event.events;

	try {
		if (events & EPOLLHUP)
			throw std::runtime_error("EPOLL hangup fd " + Utils::toString(fd) + ", events: " + Utils::toString(events));

		if (events & EPOLLERR)
			throw std::runtime_error("EPOLL error fd " + Utils::toString(fd) + ", events: " + Utils::toString(events));

		if (events & EPOLLIN)
		{
			if (std::find(_serverFds.begin(), _serverFds.end(), fd) != _serverFds.end())
				acceptClient(fd);

			if (_clients.find(fd) != _clients.end())
			{	
				_clients[fd]->updateActivity();
				_clients[fd]->getRequest()->setClientfd(fd);
				_clients[fd]->readRequest();
				if (_clients[fd]->getRequest()->isComplete()) 
				{
					_clients[fd]->getResponse()->buildResponse();
					if (!_epoll.modify(fd, EPOLLOUT)) 
						throw std::runtime_error("Failed to modify epoll to EPOLLOUT");
				}
			}
		}

		if (events & EPOLLOUT)
		{
			_clients[fd]->updateActivity();
			if (_clients[fd]->getResponse()->isReady())
				_clients[fd]->sendResponse();
			if (_clients[fd]->getResponse()->isComplete())
			{
				if (!_epoll.modify(fd, EPOLLIN))
					throw std::runtime_error("Failed to modify epoll to EPOLLIN");

				if(_clients[fd]->shouldKeepAlive())
					_clients[fd]->reset();
				else 
					cleanupClient(fd);

			}
		}
	}
	catch (const std::exception& e) 
	{
		LOG_ERROR(e.what());
		cleanupClient(fd);
	}
}



void ServerManager::acceptClient(int fd) 
{
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);

	int clientFd = accept(fd, (struct sockaddr *)&clientAddr, &clientAddrLen);

	if (clientFd == -1) 
		return;
	
	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(clientAddr.sin_addr), client_ip, INET_ADDRSTRLEN);
	int client_port = ntohs(clientAddr.sin_port);
	LOG_INFO(std::string("Client connected from ") + client_ip + ":" + Utils::toString(client_port) + " (fd " + Utils::toString(clientFd) + ")");

	if (_clients.size() >= CLIENTS)
	{
		close(clientFd);
		return;
	}

	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1) 
	{
		close(clientFd);
		return;
	}

	try 
	{
		Client* client = new Client(clientFd, _servers);

		_clients[clientFd] = client;
		if (!_epoll.add(clientFd, EPOLLIN)) 
		{
			delete client;
			close(clientFd);
			return;
		}
	} 

	catch (const std::exception& e) 
	{
		LOG_DEBUG(e.what());
		close(clientFd);
	}
}

void ServerManager::checkTimeouts()
{
	time_t currentTime = time(NULL);

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); )
	{
		if ((currentTime - it->second->getLastActivity()) >= TIMEOUT)
		{
			int fd = it->first;
			LOG_DEBUG("Client timed out  " + Utils::toString(fd));

			cleanupClient(fd);

			it = _clients.begin();
		}
		else
			++it;
	}
}

void ServerManager::cleanupClient(int fd)
{
	std::map<int, Client*>::iterator it = _clients.find(fd);

	if (it != _clients.end()) 
	{
		_epoll.remove(fd);
		delete it->second;
		_clients.erase(it);
		close(fd);
		LOG_DEBUG("Client disconnected  " + Utils::toString(fd));
	}
}
