/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zel-oirg <zel-oirg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/04 17:27:45 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/25 01:31:17 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ServerConfig.hpp"


ServerConfig::ServerConfig() : _host(""), _serverName(""), _clientMaxBodySize(0)
{
}

ServerConfig::~ServerConfig()
{
	// Close any open file descriptors
	for (size_t i = 0; i < _server_fds.size(); ++i)
		if (_server_fds[i] >= 0)
			close(_server_fds[i]);
}

ServerConfig::ServerConfig(const ServerConfig& other)
{
	*this = other;
}

ServerConfig& ServerConfig::operator=(const ServerConfig& other)
{
	if (this != &other)
	{
		_host = other._host;
		_ports = other._ports;
		_serverName = other._serverName;
		_clientMaxBodySize = other._clientMaxBodySize;
		_errorPages = other._errorPages;
		_locations = other._locations;
		_server_fds = other._server_fds;
		_server_addresses = other._server_addresses;
	}
	return *this;
}

bool ServerConfig::isValidHost(const std::string& host)
{
	struct in_addr addr;
	return (inet_pton(AF_INET, host.c_str(), &addr) == 1);
}

void ServerConfig::setHost(const std::string& host)
{
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_STREAM;

	std::string resolvedHost = host;
	if (getaddrinfo(host.c_str(), NULL, &hints, &res) == 0)
	{
		struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
		resolvedHost = inet_ntoa(ipv4->sin_addr);
		freeaddrinfo(res);
	}
	else
		throw ErrorServer("Failed to resolve hostname: " + host);

	if (!isValidHost(resolvedHost))
		throw ErrorServer("Wrong syntax: host");

	_host = resolvedHost;
}

void ServerConfig::setPort(const std::string& portStr)
{
	if (portStr.empty())
		throw ErrorServer("Wrong syntax: port (empty string)");

	for (size_t i = 0; i < portStr.length(); ++i)
		if (!std::isdigit(portStr[i]))
			throw ErrorServer("Wrong syntax: port (non-digit character)");

	std::stringstream ss(portStr);
	int port;
	ss >> port;

	if (ss.fail() || !ss.eof()) 
		throw ErrorServer("Wrong syntax: port (invalid format)");
	if (port < 1 || port > 65535)
		throw ErrorServer("Wrong syntax: port (out of range)");

	_ports.push_back(static_cast<uint16_t>(port));
}

void ServerConfig::setServerName(const std::string& name)
{
	_serverName = name;
}

void ServerConfig::setClientMaxBodySize(size_t size)
{
	_clientMaxBodySize = size;
}

void ServerConfig::setErrorPage(int code, const std::string& path)
{
	_errorPages[code] = path;
}

void ServerConfig::setFd(int fd)
{
	// This method is now outdated - kept for backward compatibility
	if (_server_fds.empty())
		_server_fds.push_back(fd);
	else
		_server_fds[0] = fd;
}

void ServerConfig::addLocation(const std::string& path, const LocationConfig& location)
{
	_locations[path] = location;
}

const std::string& ServerConfig::getHost() const
{
	return (_host);
}

uint16_t ServerConfig::getPort() const
{
	return _ports.empty() ? 0 : _ports[0];
}

const std::vector<uint16_t>& ServerConfig::getPorts() const
{
	return _ports;
}

const std::string& ServerConfig::getServerName() const
{
	return (_serverName);
}

size_t ServerConfig::getClientMaxBodySize() const
{
	return (_clientMaxBodySize);
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const
{
	return (_errorPages);
}

int ServerConfig::getFd(size_t index) const
{
	if (index >= _server_fds.size())
		throw ErrorServer("Server file descriptor index out of range");
	return _server_fds[index];
}

const std::vector<int>& ServerConfig::getFds() const
{
	return _server_fds;
}

const std::map<std::string, LocationConfig>& ServerConfig::getLocations() const
{ 
	return (_locations);
}

void ServerConfig::setupServer()
{
	// Close any existing sockets
	for (size_t i = 0; i < _server_fds.size(); ++i)
	{
		if (_server_fds[i] >= 0)
			close(_server_fds[i]);
	}
	// Clear vectors
	_server_fds.clear();
	_server_addresses.clear();

	for (size_t i = 0; i < _ports.size(); ++i)
	{
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd == -1)
			throw ErrorServer("Failed to create socket");

		int opt = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			close(fd);
			throw ErrorServer("Failed to set socket options");
		}

		struct sockaddr_in address;
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = inet_addr(_host.c_str());
		address.sin_port = htons(_ports[i]);

		if (bind(fd, (struct sockaddr*)&address, sizeof(address)) == -1)
		{
			close(fd);
			std::stringstream ss;
			ss << "Failed to bind socket on port " << _ports[i];
			throw ErrorServer(ss.str());
		}

		if (listen(fd, SOMAXCONN) == -1)
		{
			close(fd);
			std::stringstream ss;
			ss << "Failed to listen on socket on port " << _ports[i];
			throw ErrorServer(ss.str());
		}

		// Store the file descriptor and address
		_server_fds.push_back(fd);
		_server_addresses.push_back(address);
	}
}

void ServerConfig::print() const
{
	std::cout << "Server Configuration:\n";
	std::cout << "----------------------\n";
	std::cout << "Host: " << _host << "\n";

	// Print all ports
	std::cout << "Ports: ";
	for (size_t i = 0; i < _ports.size(); ++i)
	{
		if (i > 0) std::cout << ", ";
		std::cout << _ports[i];
	}
	std::cout << "\n";

	std::cout << "Server Name: " << _serverName << "\n";
	std::cout << "Client Max Body Size: " << _clientMaxBodySize << " bytes\n";

	std::cout << "Error Pages:\n";
	for (std::map<int, std::string>::const_iterator it = _errorPages.begin(); it != _errorPages.end(); ++it)
		std::cout << "  " << it->first << " -> " << it->second << "\n";

	std::cout << "Locations (" << _locations.size() << "):\n";
	for (std::map<std::string, LocationConfig>::const_iterator it = _locations.begin(); it != _locations.end(); ++it) {
		std::cout << "  Path: " << it->first << "\n";  // Show the location path
		it->second.print();  // Call print method on LocationConfig
	}
	std::cout << std::endl;
}
