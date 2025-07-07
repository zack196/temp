#include "../include/ConfigParser.hpp"
#include "../include/Utils.hpp"
#include "../include/Logger.hpp"
#include "../include/LocationConfig.hpp"
#include <cstddef>
#include <functional>
#include <sstream>
#include <string>
#include <fstream>
#include <cctype>
#include <iostream>
#include <unistd.h>

ConfigParser::ConfigParser(const std::string& configFilePath) : _configFile(configFilePath) 
{
}

void ConfigParser::validateFilePath(const std::string& path) 
{
	struct stat fileStat;
	if (stat(path.c_str(), &fileStat) != 0)
		throw std::runtime_error("Configuration file does not exist: " + path);
	if (!S_ISREG(fileStat.st_mode))
		throw std::runtime_error("Path is not a file: " + path);
	if (access(path.c_str(), R_OK) != 0)
		throw std::runtime_error("Cannot read configuration file: " + path);
	if (fileStat.st_size > 8192)
		throw std::runtime_error("Configuration file too large (max 8KB): " + path);
}

std::vector<ServerConfig> ConfigParser::getServers() const 
{
	return _servers;
}

void ConfigParser::validateBraces(const std::string& path) 
{
	std::ifstream file(path.c_str());
	if (!file.is_open())
		throw std::runtime_error("Cannot open file: " + path);

	std::string line;
	int braceCount = 0;
	int lineNumber = 0;

	while (std::getline(file, line)) 
	{
		++lineNumber;

		std::size_t commentPos = line.find('#');
		if (commentPos != std::string::npos)
			line = line.substr(0, commentPos);

		line = Utils::trimWhitespace(line);

		if (line.empty())
			continue;

		for (std::size_t i = 0; i < line.length(); ++i) 
		{
			if (line[i] == '{')
				++braceCount;
			else if (line[i] == '}') 
			{
				--braceCount;
				if (braceCount < 0)
					throw std::runtime_error("Unexpected '}' at line " + Utils::toString(lineNumber));
			}
		}

		if (line != "{" && line != "}" && line.find("server") != 0 && line.find("location") != 0)
			if (line[line.length() - 1] != ';')
				throw std::runtime_error("Missing semicolon at line " + Utils::toString(lineNumber) + line);
	}

	file.close();

	if (braceCount > 0)
		throw std::runtime_error("Unclosed opening brace(s): " + Utils::toString(braceCount));
}

void ConfigParser::validateServerConflicts() const
{
	std::map<std::string, std::string> hostPortToServerName;

	for (size_t i = 0; i < _servers.size(); ++i)
	{
		const std::map<std::string, std::vector<uint16_t> >& hostPorts = _servers[i].getHostPort();
		const std::string& serverName = _servers[i].getServerName();

		for (std::map<std::string, std::vector<uint16_t> >::const_iterator hostIt = hostPorts.begin(); hostIt != hostPorts.end(); ++hostIt)
		{
			const std::string& host = hostIt->first;
			const std::vector<uint16_t>& ports = hostIt->second;
			for (size_t j = 0; j < ports.size(); ++j)
			{
				std::string hostPort = host + ":" + Utils::toString(ports[j]);

				if (hostPortToServerName.find(hostPort) != hostPortToServerName.end())
				{
					const std::string& existingServerName = hostPortToServerName[hostPort];
					if (existingServerName == serverName)
						throw std::runtime_error("conflicting server name \"" + serverName + "\" on " + hostPort);
				}
				else
					hostPortToServerName[hostPort] = serverName;
			}
		}
	}
}


void ConfigParser::parseFile() 
{
	validateFilePath(_configFile);
	validateBraces(_configFile);

	std::ifstream file(_configFile.c_str());
	if (!file.is_open())
		throw std::runtime_error("Cannot open config file");

	parseServerBlocks(file);
	if (_servers.empty())
		throw std::runtime_error("No valid server block found in configuration file");

	validateServerConflicts();
	file.close();
	
}

std::string ConfigParser::extractLocationPath(const std::string& line) 
{
	std::size_t pos = line.find("location");
	if (pos == std::string::npos)
		return "";

	pos += 8;
	pos = line.find_first_not_of(" \t", pos);
	if (pos == std::string::npos)
		return "";

	std::size_t end = line.find('{', pos);
	if (end == std::string::npos)
		end = line.length();

	return Utils::trim(line.substr(pos, end - pos));
}


void ConfigParser::parseServerBlocks(std::ifstream& fileStream) 
{
	_servers.clear();
	std::string line, blockBuffer;
	int braceDepth = 0;
	bool serverFound = false;

	while (std::getline(fileStream, line)) 
	{
		line = Utils::trimWhitespace(line);
		if (line.empty() || line[0] == '#') 
			continue;

		if (braceDepth == 0 && !serverFound && line.find("server") != std::string::npos) 
		{
			serverFound = true;

			size_t bracePos = line.find('{');
			if (bracePos != std::string::npos) 
				braceDepth = 1;
			continue;
		}

		if (serverFound && braceDepth == 0) 
		{
			size_t bracePos = line.find('{');
			if (bracePos != std::string::npos) 
				braceDepth = 1;
			continue;
		}

		if (braceDepth > 0) 
		{
			for (size_t i = 0; i < line.length(); ++i) 
			{
				if (line[i] == '{') 
					braceDepth++;
				else if (line[i] == '}')
					braceDepth--;
			}
			if (braceDepth == 0) 
			{
				parseServerBlock(blockBuffer);
				blockBuffer.clear();
				serverFound = false;
			}
			else
			{
				if (!blockBuffer.empty()) 
					blockBuffer += '\n';
				blockBuffer += line;
			}
		}
	}
	if (braceDepth > 0)
		throw std::runtime_error("Unclosed server block");
}


void ConfigParser::parseServerBlock(const std::string& block)
{
	std::istringstream stream(block);
	std::string line;
	bool inLocation = false;
	std::string locationPath;
	std::string locationContent;
	ServerConfig server;
	bool has_default_location = false;

	while (std::getline(stream, line))
	{
		if (line.find("location") == 0)
		{
			locationPath = extractLocationPath(line);

			if (locationPath == "/")
				has_default_location = true;
			inLocation = true;
		}
		if (inLocation)
		{
			locationContent += line;
			std::size_t openBrace = locationContent.find('{');
			std::size_t closeBrace = locationContent.find('}');

			if (openBrace != std::string::npos && closeBrace != std::string::npos)
			{
				LocationConfig location;
				std::string locationBlock = locationContent.substr(openBrace + 1, closeBrace - openBrace - 1);
				location.setPath(locationPath);
				location.setRoot(server.getRoot());
				parseLocationBlock(locationBlock, location);
				server.addLocation(locationPath, location);
				inLocation = false;
				locationContent.clear();
			}
		}
		else 
		parseDirective(line, server);
	}

	if (inLocation)
		throw std::runtime_error("Unclosed location block for path: " + locationPath);
	if (!has_default_location)
	{
		LocationConfig defaultLoc;
		defaultLoc.setPath("/");
		defaultLoc.setRoot(server.getRoot());
		server.addLocation("/", defaultLoc);
	}
	_servers.push_back(server);
}


void ConfigParser::parseDirective(const std::string& line, ServerConfig& server) 
{

	std::string directiveLine = line.substr(0, line.find(';'));
	size_t spacePos = line.find_first_of(" \t");

	if (spacePos == std::string::npos) 
		throw std::runtime_error("Invalid directive format (expected 'key value;'): " + line);

	std::string key = Utils::trim(directiveLine.substr(0, spacePos));
	std::string value = Utils::trim(directiveLine.substr(spacePos + 1));

	if (key.empty()) 
		throw std::runtime_error("Empty directive key: " + line);
	if (value.empty()) 
		throw std::runtime_error("Empty directive value for key '" + key + "': " + line);

	if (key == "listen") 
		server.setHostPort(value);
	else if (key == "server_name") 
		server.setServerName(value);
	else if (key == "root") 
		server.setRoot(value);
	else if (key == "client_max_body_size") 
		server.setClientMaxBodySize(value);
	else if (key == "client_body_temp_path") 
		server.setClientBodyTmpPath(value);
	else if (key == "error_page") 
		server.setErrorPage(value);
	else
		throw std::runtime_error("Unknown server directive '" + key + "': " + line);
}

void ConfigParser::parseLocationBlock(const std::string& content, LocationConfig& location)
{
	std::istringstream stream(content);
	std::string line;

	while (std::getline(stream, line, ';'))
	{
		if (line.empty())
			continue;

		std::size_t spacePos = line.find_first_of(" \t");
		if (spacePos == std::string::npos)
			throw std::runtime_error("Invalid location directive format (expected 'key value'): " + line);

		std::string key = Utils::trim(line.substr(0, spacePos));
		std::string value = Utils::trim(line.substr(spacePos + 1));

		if (key.empty())
			throw std::runtime_error("Empty location directive key: " + line);
		if (value.empty())
			throw std::runtime_error("Empty location directive value for key '" + key + "': " + line);

		if (key == "root")
			location.setRoot(value);
		else if (key == "return")
			location.setRedirect(value);
		else if (key == "index")
			location.setIndex(value);
		else if (key == "autoindex")
			location.setAutoindex(value);
		else if (key == "allow_methods")
			location.setAllowedMethods(value);
		else if (key == "upload_path")
			location.setUploadPath(value);
		else if (key == "cgi_path")
			location.setCgiPath(value);
		else
			throw std::runtime_error("Unknown location directive '" + key + "': " + line);
	}
}





