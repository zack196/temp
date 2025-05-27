#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "LocationConfig.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include "../include/EpollManager.hpp"
#include "SessionManager.hpp"

/**
 * ServerConfig class - Handles server configuration and socket management
 */
#define CLIENT_TIMEOUT 3
#define MAX_CLIENTS 1024
#define MAX_EVENTS 1024
#define BUFFER_SIZE 1024 * 1024

class ServerConfig 
{
private:
	std::string _host;                                // Server host address
	std::vector<uint16_t> _ports;                     // Server ports
	std::string _serverName;                          // Server name
	size_t _clientMaxBodySize;                        // Maximum client body size
	std::string _clientBodyTmpPath;                   // Temporary path for client body
	std::map<int, std::string> _errorPages;           // Custom error pages by status code
	std::map<std::string, LocationConfig> _locations; // Location configurations

	std::vector<int> _server_fds;                     // Server socket file descriptors
	std::vector<struct sockaddr_in> _server_addresses;// Server socket addresses
	EpollManager* _epollManager;
	SessionManager* _sessions;

	int createSocket(uint16_t port);

	bool isValidHost(const std::string& host);

public:
	ServerConfig();
	ServerConfig(const ServerConfig& rhs);
	~ServerConfig();
	ServerConfig& operator=(const ServerConfig& rhs);

	void setHost(const std::string& host);
	void setPort(const std::string& portStr);
	void setServerName(const std::string& name);
	void setClientMaxBodySize(const std::string& size);
	void setClientBodyTmpPath(const std::string& path);
	void setErrorPage(const std::string& value);
	void addLocation(const std::string& path, const LocationConfig& location);
	const LocationConfig* findLocation(const std::string& path) const;

	const std::string& getHost() const;
	uint16_t getPort() const;
	const std::vector<uint16_t>& getPorts() const;
	const std::string& getServerName() const;
	size_t getClientMaxBodySize() const;
	std::string getClientBodyTmpPath() const;
	const std::map<int, std::string>& getErrorPages() const;
	std::string getErrorPage(int statusCode) const;
	int getFd(size_t index) const;
	const std::vector<int>& getFds() const;
	const std::map<std::string, LocationConfig>& getLocations() const;

	void setEpollManager(EpollManager* manager) { _epollManager = manager; }
	EpollManager* getEpollManager() const { return _epollManager; }

	void setupServer();
	void cleanupServer();
	void setSessionManager(SessionManager* s);
    SessionManager* getSessionManager() const;
};

#endif // SERVER_CONFIG_HPP
