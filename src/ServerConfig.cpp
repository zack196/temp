#include "../include/ServerConfig.hpp"
#include "../include/ConfigParser.hpp"
#include "../include/Utils.hpp"
#include <netinet/in.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>

ServerConfig::ServerConfig() : _host(""), _serverName("default"), _root("./www/html"), _clientMaxBodySize(1048576), _clientBodyTmpPath("/tmp"), _errorPages(), _isDefault(false)
{
}

ServerConfig::ServerConfig(const std::string& configFilePath)
{
	ConfigParser parser(configFilePath);
	parser.parseFile();
	_servers = parser.getServers();
}

ServerConfig::~ServerConfig() 
{
}

void ServerConfig::setRoot(const std::string& root)
{
	_root = root;
}

std::string ServerConfig::getRoot() const 
{
	return _root;
}


std::vector<ServerConfig> ServerConfig::getServers() const 
{
	return _servers;
}

const LocationConfig& ServerConfig::findLocation(const std::string& path) const 
{
	const std::map<std::string, LocationConfig>& locations = getLocations();
	std::map<std::string, LocationConfig>::const_iterator bestMatch = locations.end();
	size_t bestLength = 0;

	for (std::map<std::string, LocationConfig>::const_iterator it = locations.begin();
	it != locations.end(); ++it)
	{
		const std::string& locPath = it->first;
		if (path.compare(0, locPath.length(), locPath) == 0 && locPath.length() > bestLength &&
			(path.length() == locPath.length() || path[locPath.length()] == '/' || locPath == "/"))
		{
			bestMatch = it;
			bestLength = locPath.length();
		}
	}
	return bestMatch->second;
}



bool ServerConfig::isValidHost(const std::string& host) 
{
	struct in_addr addr;
	return inet_pton(AF_INET, host.c_str(), &addr) == 1;
}


void ServerConfig::setHostPort(const std::string& hostPort) 
{
	if (hostPort.empty()) 
		throw std::runtime_error("Host:Port cannot be empty");

	size_t colonPos = hostPort.find(':');
	std::string host;
	std::string portStr;

	if (colonPos == std::string::npos) 
	{
		host = "0.0.0.0";
		portStr = hostPort;
	}
	else 
	{
		host = hostPort.substr(0, colonPos);
		portStr = hostPort.substr(colonPos + 1);

		if (host.empty()) 
			throw std::runtime_error("Host cannot be empty in: " + hostPort);
		if (portStr.empty()) 
			throw std::runtime_error("Port cannot be empty after colon");
	}

	for (size_t i = 0; i < portStr.length(); ++i) 
		if (!std::isdigit(portStr[i])) 
			throw std::runtime_error("Port must contain only digits: " + portStr);

	int port = std::atoi(portStr.c_str());
	if (port < 1 || port > 65535) 
		throw std::runtime_error("Port out of range (1-65535): " + portStr);

	std::string resolved_host;

	if (isValidHost(host)) 
		resolved_host = host;
	else 
	{
		struct hostent* he = gethostbyname(host.c_str());
		if (!he || he->h_addr_list[0] == NULL)
			throw std::runtime_error("Failed to resolve hostname '" + host + "'");

		resolved_host = inet_ntoa(*reinterpret_cast<struct in_addr*>(he->h_addr_list[0]));
	}

	std::vector<uint16_t>& ports = _host_ports[resolved_host];
	for (size_t i = 0; i < ports.size(); ++i)
		if (ports[i] == port)
			throw std::runtime_error("Port " + portStr + " already assigned to host: " + resolved_host);

	ports.push_back(port);
}

std::vector<uint16_t> ServerConfig::getPortsByHost(const std::string& host) const
{
	std::map<std::string, std::vector<uint16_t> >::const_iterator it = _host_ports.find(host);
	if (it != _host_ports.end())
		return it->second;
	return std::vector<uint16_t>();
}


const std::map<std::string, std::vector<uint16_t> >& ServerConfig::getHostPort() const 
{
	return _host_ports;
}

LocationConfig& ServerConfig::getLocation(const std::string& key)
{
	std::map<std::string, LocationConfig>::iterator it = _locations.find(key);
	if (it != _locations.end()) 
		return it->second;
	else 
		throw std::runtime_error("Location key not found: " + key);
}

void ServerConfig::setServerName(const std::string& name) 
{
	if (_serverName != "default")
		throw std::runtime_error("the server name duplicated");
	_serverName = name;

}

void ServerConfig::setClientMaxBodySize(const std::string& size) 
{
	if ( _clientMaxBodySize != 1048576)
		throw std::runtime_error("the _clientMaxBodySize  duplicated");
	_clientMaxBodySize = Utils::stringToSizeT(size);
}

void ServerConfig::setClientBodyTmpPath(const std::string& path) 
{
	if ( _clientBodyTmpPath != "/tmp")
		throw std::runtime_error("the _clientBodyTmpPath duplicated");
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

	if (_errorPages.find(code) != _errorPages.end())
		throw std::runtime_error("Duplicate error code in error_page directive: " + codeStr);

	_errorPages[code] = path;
}


void ServerConfig::addLocation(std::string path, LocationConfig location)
{
    if (_locations.count(path) > 0)
        throw std::runtime_error("Duplicate location path: " + path);

    _locations[path] = location;
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
	if (it != _errorPages.end() && Utils::isFileExists(it->second))
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

const std::map<std::string, LocationConfig>& ServerConfig::getLocations() const 
{
	return _locations;
}
