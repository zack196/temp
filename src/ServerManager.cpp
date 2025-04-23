#include "../include/webserver.hpp"
#include "../include/ServerManager.hpp"
#include "../include/LocationConfig.hpp"
#include "../include/Utils.hpp"


ServerManager::ServerManager()
{
	LOG_INFO("ServerManager initialized");
}

ServerManager::~ServerManager()
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
		const std::vector<uint16_t>& ports = _servers[i].getPorts();

		for (size_t j = 0; j < fds.size(); ++j)
		{
			int listen_fd = fds[j];

			if (fcntl(listen_fd, F_SETFL, O_NONBLOCK) == -1)
			{
				LOG_ERROR("Failed to set non-blocking mode for fd: " + Utils::toString(listen_fd));
				throw std::runtime_error("fcntl failed");
			}
			_epollManager.addFd(listen_fd, EPOLLIN | EPOLLET);
			_server_map[listen_fd] = &_servers[i];

			LOG_INFO("Server created: " + _servers[i].getServerName() + " on " + _servers[i].getHost() + ":" + Utils::toString(ports[j]) + " (fd: " + Utils::toString(listen_fd) + ")");
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
			checkTimeouts(CONNECTION_TIMEOUT);
			last_timeout_check = now;
		}
		for (int i = 0; i < num_events; ++i)
		{
			try
			{
				handleEvent(events[i]);
			}
			catch (const std::exception& e)
			{
				LOG_ERROR("Error handling event: " + std::string(e.what()));
				if (_clients.find(events[i].data.fd) != _clients.end())
					closeConnection(events[i].data.fd);
			}
		}
	}
}

void ServerManager::handleEvent(struct epoll_event& event)
{
	int fd = event.data.fd;

	if (_server_map.find(fd) != _server_map.end())
		acceptConnection(*_server_map[fd], fd);  // Pass the specific fd that triggered the event
	else if (event.events & EPOLLIN)
		handleRequest(fd);
	else if (event.events & EPOLLOUT)
		handleResponse(fd);
	else if (event.events & (EPOLLRDHUP | EPOLLHUP))
	{
		LOG_DEBUG("Client disconnected (fd: " + Utils::toString(fd) + ")");
		closeConnection(fd);
	}
	else
	{
		LOG_DEBUG("Unknown event type: " + Utils::toString(event.events) + " for fd: " + Utils::toString(fd));
		closeConnection(fd);
	}
}

void ServerManager::acceptConnection(ServerConfig& server, int server_fd)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	memset(&client_addr, 0, client_len);

	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd == -1)
		throw ErrorServer("Accept failed");

	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
		throw ErrorServer("Failed to set client socket non-blocking");

	_epollManager.addFd(client_fd, EPOLLIN | EPOLLET | EPOLLRDHUP);

	_clients[client_fd] = new Client(client_fd, &server);

	char ip_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);

	// Find which port this connection came in on
	uint16_t port = 0;
	const std::vector<int>& fds = server.getFds();
	const std::vector<uint16_t>& ports = server.getPorts();
	for (size_t i = 0; i < fds.size(); ++i) {
		if (fds[i] == server_fd) {
			port = ports[i];
			break;
		}
	}
	LOG_INFO("Accepted new connection from " + std::string(ip_str) + " on server " + server.getServerName() + " port " + Utils::toString(port) + " (fd: " + Utils::toString(client_fd) + ")");
}

void ServerManager::handleRequest(int client_fd)
{
	char buffer[BUFFER_SIZE + 1];
	ssize_t bytes_read;
	this->_clients[client_fd]->updateActivity();

	bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

	if (bytes_read > 0)
	{
		LOG_DEBUG("[handleRequest] Received " + Utils::toString(bytes_read) + " bytes from client " + Utils::toString(client_fd));
		buffer[bytes_read] = '\0';
	}
	else if (bytes_read < 0)
		throw ErrorServer("Error with recv function");
	else if (bytes_read == 0)
	{
		LOG_INFO("Client disconnected (fd: " + Utils::toString(client_fd) + ")");
		return(closeConnection(client_fd));
	}
	this->_clients[client_fd]->parseRequest(buffer, bytes_read);
	_epollManager.modifyFd(client_fd, EPOLLOUT | EPOLLET);
}

void ServerManager::handleResponse(int client_fd)
{
	LOG_INFO("Client prepare response (fd: " + Utils::toString(client_fd) + ")");

	this->_clients[client_fd]->updateActivity();

	this->_clients[client_fd]->handleResponse();
	LOG_DEBUG("Sent response (fd: " + Utils::toString(client_fd) + ")");

	closeConnection(client_fd);
}

void ServerManager::closeConnection(int client_fd)
{
	_epollManager.removeFd(client_fd);

	close(client_fd);

	delete _clients[client_fd];
	_clients.erase(client_fd);

	LOG_DEBUG("Closed connection (fd: " + Utils::toString(client_fd) + ")");
}

void ServerManager::checkTimeouts(int timeout_seconds)
{
	time_t now = time(NULL);
	std::vector<int> toClose;

	// Find all clients that have timed out
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (now - it->second->getLastActivity() > timeout_seconds)
			toClose.push_back(it->first);

	// Close all timed out connections
	for (size_t i = 0; i < toClose.size(); ++i)
	{
		LOG_INFO("Closing idle connection (fd: " + Utils::toString(toClose[i]) + ") due to timeout");
		closeConnection(toClose[i]);
	}
}
