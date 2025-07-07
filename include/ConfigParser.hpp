#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <fstream>
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

class ConfigParser
{
public:
	ConfigParser(const std::string& configFilePath);

	void parseFile();
	std::vector<ServerConfig> getServers() const;

private:
	std::string _configFile;
	std::vector<ServerConfig> _servers;

	void validateFilePath(const std::string& path);
	void validateBraces(const std::string& path);
	void validateServerConflicts() const;

	void parseServerBlocks(std::ifstream& fileStream);
	void parseServerBlock(const std::string& block);
	void parseDirective(const std::string& line, ServerConfig& server);
	void parseLocationBlock(const std::string& content, LocationConfig& location);

	std::string extractLocationPath(const std::string& line);
};

#endif

