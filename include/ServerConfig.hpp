#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <string>
# include <vector>
# include <map>
# include <cstdlib>
# include <cctype>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>
# include "LocationConfig.hpp"
# include "Utils.hpp"

#define TIMEOUT 30
#define CGI_TIMEOUT 10
#define CLIENTS 1024
#define EVENTS 1024
#define BUFFER_SIZE 1024*1024

class ServerConfig 
{
private:
	std::string _host;
	std::string _serverName;
	std::string _root;
	size_t _clientMaxBodySize;
	std::string _clientBodyTmpPath;
	std::map<int, std::string> _errorPages;
	std::map<std::string, LocationConfig> _locations;
	std::map<std::string, std::vector<uint16_t> > _host_ports;
	std::vector<ServerConfig> _servers;
	bool _isDefault;

public:
	ServerConfig();
	ServerConfig(const std::string& configFilePath);
	~ServerConfig();

	void setRoot(const std::string& root);
	std::string getRoot() const;

	std::vector<ServerConfig> getServers() const;

	void setPorts(const std::vector<uint16_t>& newPorts);
	std::vector<uint16_t> getPortsByHost(const std::string& host) const;
	const std::map<std::string, std::vector<uint16_t> >& getHostPort() const;

	void setHostPort(const std::string& hostPort);
	static bool isValidHost(const std::string& host);

	void setServerName(const std::string& name);
	const std::string& getServerName() const;

	void setClientMaxBodySize(const std::string& size);
	size_t getClientMaxBodySize() const;

	void setClientBodyTmpPath(const std::string& path);
	std::string getClientBodyTmpPath() const;

	void setErrorPage(const std::string& value);
	const std::map<int, std::string>& getErrorPages() const;
	std::string getErrorPage(int statusCode) const;

	void addLocation(std::string path, LocationConfig location);
	const std::map<std::string, LocationConfig>& getLocations() const;
	LocationConfig& getLocation(const std::string& key);
	const LocationConfig& findLocation(const std::string& path) const;
};

#endif
