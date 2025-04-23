/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zel-oirg <zel-oirg@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/04 17:29:36 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/18 16:17:33 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ConfigParser.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

ConfigParser::ConfigParser() : _configFile() 
{
}

ConfigParser::ConfigParser(const std::string& configFilePath) : _configFile(configFilePath)
{
}

ConfigParser::ConfigParser(const ConfigParser& other) 
{
	*this = other;
}

ConfigParser& ConfigParser::operator=(const ConfigParser& other) 
{
	if (this != &other) 
	{
		_configFile = other._configFile;
		_servers = other._servers;
	}
	return *this;
}

ConfigParser::~ConfigParser() 
{
}

const std::vector<ServerConfig>& ConfigParser::getServers()
{
	return _servers;
}

void ConfigParser::removeComments(std::string& content) 
{
	size_t pos = 0;
	while ((pos = content.find('#', pos)) != std::string::npos) 
	{
		size_t end = content.find('\n', pos);
		content.erase(pos, end != std::string::npos ? end - pos : std::string::npos);
	}
}

// Utility function to trim leading and trailing whitespace
void ConfigParser::trimWhitespace(std::string& content) 
{
	// Trim leading whitespace
	size_t pos = content.find_first_not_of(" \t\r\n");
	if (pos != std::string::npos) 
		content.erase(0, pos);
	else
	{
		content.clear();
		return;  // Entire string was whitespace
	}

	// Trim trailing whitespace
	pos = content.find_last_not_of(" \t\r\n");
	if (pos != std::string::npos) 
		content.erase(pos + 1);
}

// Utility function to collapse multiple whitespaces into a single space
void ConfigParser::collapseSpaces(std::string& content) 
{
	std::string cleaned;
	bool inSpace = false;
	bool inNewline = false;

	for (size_t i = 0; i < content.size(); ++i) 
	{
		if (content[i] == '\n') 
		{
			if (!inNewline) 
			{
				cleaned += '\n';
				inNewline = true;
				inSpace = false;  // Newline takes precedence
			}
		}
		else if (std::isspace(content[i])) 
		{
			if (!inSpace && !inNewline) 
			{
				cleaned += ' ';
				inSpace = true;
			}
		}
		else 
		{
			cleaned += content[i];
			inSpace = false;
			inNewline = false;
		}
	}

	// Remove trailing spaces or newlines
	size_t pos = cleaned.find_last_not_of(" \t\r\n");
	if (pos != std::string::npos) 
		cleaned.erase(pos + 1);
	else 
		cleaned.clear();

	content = cleaned;
}

void ConfigParser::cleanContent(std::string& content) 
{
	removeComments(content);

	trimWhitespace(content);

	collapseSpaces(content);
}

void ConfigParser::parseFile() 
{
	const std::string& path = _configFile.getPath();

	validateFilePath(path);

	std::string content = readFileContent(path);

	cleanContent(content);

	parseServerBlocks(content);
}

void ConfigParser::validateFilePath(const std::string& path)
{
	switch (_configFile.getTypePath(path)) 
	{
		case 0: 
			throw std::runtime_error("Configuration file does not exist: " + path);
		case 2: 
			throw std::runtime_error("Path is a directory, not a file: " + path);
		case 3: 
			throw std::runtime_error("Path is a special file: " + path);
		case 1: 
			break;
		default: 
			throw std::runtime_error("Unknown file type: " + path);
	}

	if (!_configFile.checkFile(path, 0)) 
		throw std::runtime_error("No read permission for file: " + path);
}

std::string ConfigParser::readFileContent(const std::string& path)
{
	std::string content;
	if (!_configFile.readFile(path, content)) 
		throw std::runtime_error("Failed to read file: " + path);

	if (content.empty()) 
		throw std::runtime_error("File is empty: " + path);

	return content;
}

void ConfigParser::parseServerBlocks(const std::string& content)
{

	size_t pos = 0;
	while ((pos = content.find("server", pos)) != std::string::npos) 
	{
		size_t blockStart = findBlockStart(content, pos);

		size_t blockEnd = findBlockEnd(content, blockStart);

		std::string block = content.substr(blockStart + 1, blockEnd - blockStart - 2);

		parseServerBlock(block);

		pos = blockEnd;
	}
}

size_t ConfigParser::findBlockStart(const std::string& content, size_t pos)
{
	size_t blockStart = content.find("{", pos);
	if (blockStart == std::string::npos) 
		throw std::runtime_error("Missing '{' in server block");

	return blockStart;
}

size_t ConfigParser::findBlockEnd(const std::string& content, size_t blockStart)
{
	int braceCount = 1;
	size_t blockEnd = blockStart + 1;

	// Count the number of braces to find the end of the server block
	while (braceCount > 0 && blockEnd < content.size()) 
	{
		if (content[blockEnd] == '{') 
			braceCount++;
		else if (content[blockEnd] == '}') 
			braceCount--;
		blockEnd++;
	}

	if (braceCount != 0) 
		throw std::runtime_error("Unmatched '{' in server block");

	return (blockEnd);
}

// Helper function to parse a single server block
void ConfigParser::parseServerBlock(const std::string& block) 
{
	ServerConfig server;
	size_t pos = 0;

	while (pos < block.size()) 
	{
		while (pos < block.size() && std::isspace(block[pos]))
			++pos;
		if (pos >= block.size())
			break;

		size_t endDirective = block.find_first_of(";\n{", pos);
		if (endDirective == std::string::npos)
			break;

		std::string directiveLine = trim(block.substr(pos, endDirective - pos));
		if (directiveLine.empty()) 
		{
			pos = endDirective + 1;
			continue;
		}

		size_t spacePos = directiveLine.find_first_of(" \t");
		if (spacePos == std::string::npos)
		{
			pos = endDirective + 1;
			continue;
		}
		std::string key = trim(directiveLine.substr(0, spacePos));
		std::string value = trim(directiveLine.substr(spacePos + 1));

		if (key == "location")
		{
			size_t openBrace = block.find('{', endDirective);
			if (openBrace == std::string::npos)
				throw std::runtime_error("Missing '{' in location block");

			int braceCount = 1;
			size_t closeBrace = openBrace + 1;
			while (braceCount > 0 && closeBrace < block.size()) 
			{
				if (block[closeBrace] == '{')
					braceCount++;
				else if (block[closeBrace] == '}')
					braceCount--;
				closeBrace++;
			}
			if (braceCount != 0)
				throw std::runtime_error("Unmatched '{' in location block");
			std::string locationBlock = block.substr(openBrace + 1, closeBrace - openBrace - 2);
			LocationConfig location = parseLocationBlock(value, locationBlock);
			server.addLocation(value, location);
			pos = closeBrace;
		} 
		else 
		{
			if (key == "host")
				server.setHost(value);
			else if (key == "listen")
				server.setPort(value);
			else if (key == "server_name")
				server.setServerName(value);
			else if (key == "client_max_body_size")
				server.setClientMaxBodySize(std::atoi(value.c_str()));
			else if (key == "error_page") 
			{
				std::vector<std::string> tokens = split(value, ' ');
				if (tokens.size() == 2)
					server.setErrorPage(std::atoi(tokens[0].c_str()), tokens[1]);
			}
			pos = endDirective + 1;
		}
	}

	_servers.push_back(server);
}

LocationConfig ConfigParser::parseLocationBlock(const std::string& locationHeader, const std::string& block) 
{
	LocationConfig location;
	std::istringstream iss(block);
	std::string line;


	location.setPath(locationHeader);

	while (std::getline(iss, line))
	{
		line = trim(line);
		if (line.empty() || line == "}")
			continue;

		// Parse the key-value pair directly here
		size_t pos = line.find_first_of(" \t");
		if (pos == std::string::npos)
			throw std::runtime_error("Invalid config line: " + line);

		std::string key = trim(line.substr(0, pos));
		std::string value = trim(line.substr(pos + 1));

		// Remove the trailing semicolon or brace if present
		if (!value.empty() && (value[value.size() - 1] == ';' || value[value.size() - 1] == '{'))
			value = value.substr(0, value.size() - 1);

		// Assign values based on the key
		if (key == "root")
			location.setRoot(value);
		else if (key == "index")
			location.setIndex(value);
		else if (key == "autoindex")
			location.setAutoindex(value);
		else if (key == "allow_methods")
			location.setAllowedMethods(value);
		else if (key == "cgi_extension")
			location.setCgiExtension(value);
		else if (key == "cgi_path")
			location.setCgiPath(value);
	}

	return (location);
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter) 
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream iss(str);
	while (std::getline(iss, token, delimiter)) 
	{
		token = trim(token);
		if (!token.empty())
			tokens.push_back(token);
	}
	return (tokens);
}

std::string ConfigParser::trim(const std::string& str) 
{
	size_t first = str.find_first_not_of(" \t");
	size_t last = str.find_last_not_of(" \t");
	if (first == std::string::npos || last == std::string::npos)
		return "";
	return str.substr(first, last - first + 1);
}

void ConfigParser::print() const 
{
	for (size_t i = 0; i < _servers.size(); ++i) 
	{
		std::cout << "Server " << i + 1 << ":\n";
		_servers[i].print();
	}
}
