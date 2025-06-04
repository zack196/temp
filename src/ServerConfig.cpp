#include "../include/ServerConfig.hpp"
#include "../include/Utils.hpp"
#include <netinet/in.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdexcept>

ServerConfig::ServerConfig() : _host("0.0.0.0"), _serverName(""), _clientMaxBodySize(1024 * 1024) //1MB
{
}

ServerConfig::~ServerConfig() 
{
	cleanupServer();
}

ServerConfig::ServerConfig(const ServerConfig& rhs) : _host(""), _serverName(""), _clientMaxBodySize(0)
{
	*this = rhs;
}

ServerConfig& ServerConfig::operator=(const ServerConfig& rhs) 
{
	if (this != &rhs) 
	{
		cleanupServer();

		_host = rhs._host;
		_ports = rhs._ports;
		_serverName = rhs._serverName;
		_clientMaxBodySize = rhs._clientMaxBodySize;
		_clientBodyTmpPath = rhs._clientBodyTmpPath;
		_errorPages = rhs._errorPages;
		_locations = rhs._locations;
	}
	return *this;
}

const LocationConfig* ServerConfig::findLocation(const std::string& path) const 
{
	const std::map<std::string, LocationConfig>& locations = getLocations();
	const LocationConfig* bestMatch = NULL;
	size_t bestLength = 0;
	for (std::map<std::string, LocationConfig>::const_iterator it = locations.begin();
	it != locations.end(); ++it)
	{
		const std::string& locPath = it->first;
		if (path.compare(0, locPath.length(), locPath) == 0 &&
			locPath.length() > bestLength &&
			(path.length() == locPath.length() || path[locPath.length()] == '/' || locPath == "/"))
		{
			bestMatch = &it->second;
			bestLength = locPath.length();
		}
	}
	return bestMatch;
}

bool ServerConfig::isValidHost(const std::string& host) 
{
	struct in_addr addr;
	return inet_pton(AF_INET, host.c_str(), &addr) == 1;
}

void ServerConfig::setHost(const std::string& host) 
{
	if (host.empty()) 
	{
		_host = "0.0.0.0";
		return;
	}

	if (isValidHost(host)) 
	{
		_host = host;
		return;
	}

	// Otherwise, try to resolve the hostname to an IP address
	struct addrinfo hints, *res = NULL;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(host.c_str(), NULL, &hints, &res);
	if (status != 0) 
	{
		throw std::runtime_error("Failed to resolve hostname '" + host + "': " + gai_strerror(status));
	}

	if (res != NULL) 
	{
		struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
		_host = inet_ntoa(ipv4->sin_addr);
		freeaddrinfo(res);
	} 
	else 
	{
		freeaddrinfo(res);
		throw std::runtime_error("No addresses found for host: " + host);
	}
}

void ServerConfig::setPort(const std::string& portStr) 
{
	if (portStr.empty()) 
		throw std::runtime_error("Port cannot be empty");

	for (size_t i = 0; i < portStr.length(); ++i) 
	{
		if (!isdigit(portStr[i])) 
			throw std::runtime_error("Port must contain only digits: " + portStr);
	}

	// Convert to integer and validate range
	int port = std::atoi(portStr.c_str());
	if (port < 1 || port > 65535) 
		throw std::runtime_error("Port out of range (1-65535): " + portStr);

	_ports.push_back(port);
}

void ServerConfig::setServerName(const std::string& name) 
{
	_serverName = name;
}

void ServerConfig::setClientMaxBodySize(const std::string& size) 
{
	_clientMaxBodySize = Utils::stringToSizeT(size);
}

void ServerConfig::setClientBodyTmpPath(const std::string& path) 
{
	_clientBodyTmpPath = path;
}

std::string ServerConfig::getClientBodyTmpPath() const 
{
	return _clientBodyTmpPath;
}

void ServerConfig::setErrorPage(const std::string& value) 
{
	std::istringstream iss(value);
	std::string codeStr;
	std::string path;

	if (!(iss >> codeStr)) 
		throw std::runtime_error("Missing error code in error_page directive");

	int code;
	std::istringstream codeStream(codeStr);
	if (!(codeStream >> code) || code < 100 || code > 599)
		throw std::runtime_error("Invalid HTTP error code: " + codeStr);

	if (!(iss >> path)) 
		throw std::runtime_error("Missing path in error_page directive");

	_errorPages[code] = path;
}

void ServerConfig::addLocation(const std::string& path, const LocationConfig& location) 
{
	_locations[path] = location;
}

const std::string& ServerConfig::getHost() const 
{
	return _host;
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
	return _serverName;
}

size_t ServerConfig::getClientMaxBodySize() const 
{
	return _clientMaxBodySize;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const 
{
	return _errorPages;
}

std::string ServerConfig::getErrorPage(int statusCode) const 
{
	std::string message = Utils::getMessage(statusCode);

	std::map<int, std::string>::const_iterator it = _errorPages.find(statusCode);
	if (it != _errorPages.end() && Utils::fileExists(it->second))
		return it->second;

	std::ostringstream oss;
	oss << "<!DOCTYPE html>\n<html>\n"
		<< "<head><title>" << statusCode << " " << message << "</title></head>\n"
		<< "<body>\n"
		<< "<h1><center>" << statusCode << " " << message << "</center></h1>\n"
		<< "<hr><center>Webserver 1337/1.0</center>\n"
		<< "</body>\n</html>\n";

	return oss.str();
}

int ServerConfig::getFd(size_t index) const 
{
	if (index >= _server_fds.size()) 
		throw std::runtime_error("File descriptor index out of range");

	return _server_fds[index];
}

const std::vector<int>& ServerConfig::getFds() const 
{
	return _server_fds;
}

const std::map<std::string, LocationConfig>& ServerConfig::getLocations() const 
{
	return _locations;
}

void ServerConfig::setupServer() 
{
	cleanupServer();

	for (std::vector<uint16_t>::const_iterator it = _ports.begin(); it != _ports.end(); ++it) 
	{
		try 
		{
			int fd = createSocket(*it);
			_server_fds.push_back(fd);
		} 
		catch (const std::exception& e) 
		{
			cleanupServer();
			throw; // Re-throw the exception
		}
	}
}

void ServerConfig::cleanupServer() 
{
	for (std::vector<int>::iterator it = _server_fds.begin(); it != _server_fds.end(); ++it) 
	{
		if (*it >= 0) 
			if (close(*it) == -1)
				std::cerr << "Warning: Failed to close socket: " << *it << " (errno: " << errno << ")" << std::endl;
	}
	_server_fds.clear();
	_server_addresses.clear();
}

int ServerConfig::createSocket(uint16_t port) 
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) 
		throw std::runtime_error(std::string("Failed to create socket: ") + strerror(errno));

	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) 
	{
		close(fd);
		throw std::runtime_error(std::string("Failed to set socket options: "));
	}

	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) 
	{
		int save_errno = errno;
		close(fd);
		throw std::runtime_error(std::string("Failed to set non-blocking mode: ") +  strerror(save_errno));
	}

	struct sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) == -1) 
	{
		int save_errno = errno;
		close(fd);
		throw std::runtime_error("Failed to bind socket on port " + Utils::toString(port) + ": " + strerror(save_errno));
	}

	if (listen(fd, SOMAXCONN) == -1) 
	{
		int save_errno = errno;
		close(fd);
		throw std::runtime_error("Failed to listen on socket on port " + Utils::toString(port) + ": " + strerror(save_errno));
	}
	_server_addresses.push_back(address);

	return fd;
}
