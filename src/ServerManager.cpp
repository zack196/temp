#include "../include/ServerManager.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/HTTPResponse.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Utils.hpp"
#include "../include/Logger.hpp"
#include "../include/Client.hpp"
#include "../include/SessionManager.hpp"
#include <cstddef>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>


ServerManager::ServerManager(const std::vector<ServerConfig>& servers) : _epollManager(MAX_EVENTS), _running(false), _clientTimeout(CLIENT_TIMEOUT), _maxClients(MAX_CLIENTS), _servers(servers)
{
	LOG_INFO("ServerManager initialized with timeout=" + Utils::toString(_clientTimeout) + "s, maxClients=" + Utils::toString(_maxClients));
}

ServerManager::~ServerManager()
{
	LOG_INFO("ServerManager shutting down");
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}
	_clients.clear();
}

bool ServerManager::init()
{
	for (size_t i = 0; i < _servers.size(); ++i)
	{
		try 
		{
			_servers[i].setupServer();
			const std::vector<int>& fds = _servers[i].getFds();
			const std::vector<uint16_t>& ports = _servers[i].getPorts();
			for (size_t j = 0; j < fds.size(); ++j)
			{
				_serverMap[fds[j]] = &_servers[i];
				if (!_epollManager.add(fds[j], EPOLLIN))
					return false;
				LOG_INFO("Server socket " + Utils::toString(fds[j]) + " listening on " 
					+ _servers[i].getHost() + ":" + Utils::toString(ports[j]));
			}
		}
		catch (const std::exception& e)
		{
			LOG_ERROR("Error setting up server: " + std::string(e.what()));
			return false;
		}
	}
	return true;
}

void ServerManager::run()
{
	_running = true;
	LOG_INFO("Starting server event loop");

	while (_running)
	{
		int numEvents = _epollManager.wait(1000);
		if (numEvents < 0)
		{
			if (errno == EINTR)
				continue;
			LOG_ERROR("Epoll wait error: " + std::string(std::strerror(errno)));
			break;
		}
		for (int i = 0; i < numEvents; ++i)
		{
			const epoll_event& event = _epollManager.getEvent(i);
			handleEvent(event);
		}
		checkClientTimeouts();
	}
	LOG_INFO("Server event loop stopped");
}

void ServerManager::stop()
{
	LOG_INFO("Stopping server");
	_running = false;
}

void ServerManager::handleEvent(const struct epoll_event& event)
{
	int fd = event.data.fd;
	uint32_t events = event.events;
	ServerConfig* server = findServerByFd(fd);

	if (server)
	{
		if (events & EPOLLIN)
			if (!acceptClient(fd, server))
				LOG_ERROR("Failed to accept client on server fd: " + Utils::toString(fd));
		return;
	}

	Client* client = findClientByFd(fd);
	if (!client)
	{
		_epollManager.remove(fd);
		return;
	}

	if (events & (EPOLLERR | EPOLLHUP))
	{
		cleanupClient(fd, (events & EPOLLERR) ? "Socket error" : "Connection hangup");
		return;
	}

	if (events & EPOLLIN)
	{
		if (client->isCgi())
			handleCgiResponse(client);
		else
		{
			if (!receiveFromClient(client))
				return(cleanupClient(fd, "Connection complete"));

			if (client->getRequest()->hasCgi() && client->getRequest()->isComplete())
				startCgi(client);
		}
	}
	else if ((events & EPOLLOUT))
	{
		if (!sendToClient(client))
			return(cleanupClient(fd, "Connection complete"));
	}
}


bool ServerManager::acceptClient(int serverFd, ServerConfig* serverConfig)
{
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen);
	if (clientFd == -1)
		return false;

	if (_clients.size() >= static_cast<size_t>(_maxClients))
	{
		LOG_WARN("Max clients reached (" + Utils::toString(_maxClients) + "), rejecting new connection");
		close(clientFd);
		return false;
	}

	int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		close(clientFd);
		return false;
	}
	try
	{
		Client* client = new Client(clientFd, serverConfig);
		_clients[clientFd] = client;
		if (!_epollManager.add(clientFd, EPOLLIN))
		{
			delete client;
			_clients.erase(clientFd);
			close(clientFd);
			return false;
		}
		std::string clientIp = inet_ntoa(clientAddr.sin_addr);
		int clientPort = ntohs(clientAddr.sin_port);
		LOG_INFO("New client connected from " + clientIp + ":" + Utils::toString(clientPort) + " (fd: " + Utils::toString(clientFd) + ", total: " + Utils::toString(_clients.size()) + ")");
		return true;
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Error creating client: " + std::string(e.what()));
		close(clientFd);
		return false;
	}
}

bool ServerManager::receiveFromClient(Client* client)
{
	if (!client)
		return false;

	char buffer[BUFFER_SIZE];
	std::memset(buffer, 0, BUFFER_SIZE);
	ssize_t bytesRead = recv(client->getFd(), buffer, sizeof(buffer) - 1, 0);


	// --------------- handel if the session exist and get /login -----------------
	Session& sess = *client->getSession();
	HTTPRequest	*req = client->getRequest();
	
	if ( req->getMethod() == "GET"
	     && req->getPath() == "/login"
	     && sess.data.count("user") )            // ← already authenticated
	{
	    HTTPResponse* res = client->getResponse();

	    res->setProtocol(req->getProtocol());
	    res->setStatusCode(302);                 // 302 Found
	    res->setStatusMessage("Found");
	    res->setHeader("Location", "/");         // where to send him
	    res->setHeader("Content-Length", "0");
	    res->setHeader("Connection",
	                   res->shouldCloseConnection() ? "close" : "keep-alive");
	    res->buildHeader();                      // no body, header only

	    // flip the socket to EPOLLOUT right here
	    _epollManager.modify(client->getFd(), EPOLLOUT);
	    return true;                             // short-circuit normal buildResponse()
	}

	if (bytesRead > 0)
	{
		client->updateActivity();
		client->setReadBuffer(buffer, bytesRead);
		client->getRequest()->parse(client->getReadBuffer());
		
		if (client->getRequest()->isComplete() && !client->getRequest()->hasCgi())
		{
			client->setSession(&SessionManager::get().acquire(client->getRequest(), client->getResponse()));

			// print the sid of all session in webserv
			SessionManager::get().printSession();
			// login logic

			if (LoginController::handle(client->getRequest(), client->getResponse(), *client->getSession()))
				client->getResponse()->buildHeader();
			else
				client->getResponse()->buildResponse();

			if (!_epollManager.modify(client->getFd(), EPOLLOUT))
				return false;
		}
		return true;
	}

	if (bytesRead == 0)
	{
		LOG_INFO("Client fd " + Utils::toString(client->getFd()) + " closed connection");
		return false;
	}
	LOG_ERROR("Error receiving from client fd " + Utils::toString(client->getFd()) + ": " + std::string(std::strerror(errno)));
	return false;
}

bool	ServerManager::sendToClient(Client* client)
{
	if (!client)
		return false;

	char buffer[BUFFER_SIZE];
	ssize_t bytesToSend = client->getResponseChunk(buffer, sizeof(buffer));
	if (bytesToSend > 0)
	{
		ssize_t bytesSent = send(client->getFd(), buffer, bytesToSend, 0);
		if (bytesSent <= 0)
		{
			LOG_ERROR("Error sending to client fd " + Utils::toString(client->getFd()) + ": " + std::string(std::strerror(errno)));
			return false;
		}
		return true;
	}

	else if (bytesToSend == 0)
	{
		if (!client->shouldKeepAlive())
			return (LOG_INFO("closing connection for client fd " + Utils::toString(client->getFd())), false);

		if (!_epollManager.modify(client->getFd(), EPOLLIN))
			return false;
		client->reset();
		LOG_INFO("Keeping connection alive for client fd " + Utils::toString(client->getFd()));
		return true;
	}
	LOG_ERROR("Error generating response for client fd "+ Utils::toString(client->getFd()));
	return false;
}

void ServerManager::startCgi(Client* client)
{
	try
	{
		CGIHandler* cgiHandler = new CGIHandler(client->getRequest(), client->getResponse());
		cgiHandler->setEnv();
		cgiHandler->start();
		client->updateActivity();
		client->setCgiHandler(cgiHandler);
		client->setCgiStartTime(time(NULL));
		client->setIsCgi(true);

		_epollManager.add(cgiHandler->getPipeFd(), EPOLLIN);
		LOG_INFO("CGI process started for URI: " + client->getRequest()->getUri());
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Error starting CGI process: " + std::string(e.what()));
		cleanupClient(client->getFd(), "Failed to start CGI process");
	}
}

void ServerManager::handleCgiResponse(Client* client)
{
	if (!client || !client->isCgi())
		return;

	char buffer[BUFFER_SIZE] = {0};
	ssize_t bytesRead;

	try 
	{
		while ((bytesRead = read(client->getCgiHandler()->getPipeFd(), buffer, BUFFER_SIZE - 1)) > 0)
		{
			client->updateActivity();
			client->getResponse()->parseCgiOutput(std::string(buffer, bytesRead));
			std::memset(buffer, 0, BUFFER_SIZE);  // Clear buffer for next read
		}
		if (bytesRead <= 0) 
		{
			_epollManager.remove(client->getCgiHandler()->getPipeFd());
			client->getResponse()->buildResponse();
			client->setIsCgi(false);

			if (!_epollManager.modify(client->getFd(), EPOLLOUT))
				throw std::runtime_error("Failed to modify client to EPOLLOUT mode");

			LOG_INFO("CGI response completed for client fd: " + Utils::toString(client->getFd()));
		}
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Failed to handle CGI response: " + std::string(e.what()));
		cleanupClient(client->getFd(), "Error during CGI response handling: " + std::string(e.what()));
	}
}

void ServerManager::cleanupClient(int fd, const std::string& reason)
{
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it != _clients.end())
	{
		Client* client = it->second;
		if (client->isCgi() && client->getCgiHandler() != NULL)
		{
			_epollManager.remove(client->getCgiHandler()->getPipeFd());
			client->getCgiHandler()->cleanup();
			LOG_INFO("Cleaned up CGI process for client fd: " + Utils::toString(fd) + ", reason: " + reason);
		}

		LOG_INFO("Client disconnected (fd: " + Utils::toString(fd) + ", reason: " + reason + ", remaining: " + Utils::toString(_clients.size() - 1) + ")");
		delete it->second;
		_clients.erase(it);
	}

	_epollManager.remove(fd);
	close(fd);
}

void ServerManager::checkClientTimeouts()
{
	if (_clientTimeout <= 0)
		return;

	time_t currentTime = time(NULL);
	std::vector<int> timeoutFds;

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		Client* client = it->second;
		if (client->isCgi())
		{
			if ((currentTime - client->getCgiStartTime()) >= _clientTimeout)
				timeoutFds.push_back(it->first);
		}
		else
	{
			if ((currentTime - client->getLastActivity()) >= _clientTimeout)
				timeoutFds.push_back(it->first);
		}
	}

	for (size_t i = 0; i < timeoutFds.size(); ++i)
		cleanupClient(timeoutFds[i], "Connection timed out");

	if (!timeoutFds.empty())
		LOG_INFO("Cleaned up " + Utils::toString(timeoutFds.size()) + " client(s) due to " + Utils::toString(_clientTimeout) + "s timeout");

	// delete the expired session
	SessionManager::get().reapExpired(currentTime);
}

ServerConfig* ServerManager::findServerByFd(int fd)
{
	std::map<int, ServerConfig*>::iterator it = _serverMap.find(fd);
	if (it != _serverMap.end())
		return it->second;
	return NULL;
}

Client* ServerManager::findClientByFd(int fd)
{
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it != _clients.end())
		return it->second;

	for (std::map<int, Client*>::iterator cIt = _clients.begin(); cIt != _clients.end(); ++cIt)
	{
		Client* c = cIt->second;
		if (c->isCgi() && c->getCgiHandler() && c->getCgiHandler()->getPipeFd() == fd)
			return c;
	}
	return NULL;
}

void ServerManager::setClientTimeout(int seconds)
{
	_clientTimeout = seconds;
	LOG_INFO("Client timeout set to " + Utils::toString(seconds) + " seconds");
}

size_t ServerManager::getActiveClientCount() const
{
	return _clients.size();
}
