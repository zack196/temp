#include "../include/ConfigParser.hpp"
#include "../include/Utils.hpp"
#include <sstream>
#include <cctype>

ConfigParser::ConfigParser() : _configFile(), _servers() 
{
}

ConfigParser::ConfigParser(const std::string& configFilePath) : _configFile(configFilePath), _servers() 
{
}

ConfigParser::ConfigParser(const ConfigParser& rhs) : _configFile(rhs._configFile), _servers(rhs._servers)
{
}

ConfigParser& ConfigParser::operator=(const ConfigParser& rhs) 
{
	if (this != &rhs) 
	{
		_configFile = rhs._configFile;
		_servers = rhs._servers;
	}
	return *this;
}

void ConfigParser::parseFile() 
{
	const std::string& path = _configFile.getPath();
	validateFilePath(path);

	std::string content = _configFile.readFile(path);
	cleanContent(content);
	parseServerBlocks(content);
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

void ConfigParser::trimWhitespace(std::string& content) 
{
	// Remove leading whitespace
	size_t start = content.find_first_not_of(" \t\r\n");
	if (start != std::string::npos) 
		content.erase(0, start);
	else 
	{
		content.clear();
		return;
	}

	// Remove trailing whitespace
	size_t end = content.find_last_not_of(" \t\r\n");
	if (end != std::string::npos) 
		content.erase(end + 1);
}

void ConfigParser::collapseSpaces(std::string& content) 
{
	std::string result;
	result.reserve(content.size());  // Optimize for performance

	bool inSpace = false;
	bool inNewline = false;

	for (size_t i = 0; i < content.size(); ++i) 
	{
		if (content[i] == '\n') 
		{
			if (!inNewline) 
			{
				result += '\n';
				inNewline = true;
				inSpace = false;
			}
		}
		else if (std::isspace(content[i])) 
		{
			if (!inSpace && !inNewline) 
			{
				result += ' ';
				inSpace = true;
			}
		}
		else 
		{
			result += content[i];
			inSpace = false;
			inNewline = false;
		}
	}

	size_t pos = result.find_last_not_of(" \t\r\n");
	if (pos != std::string::npos) 
		result.erase(pos + 1);

	else 
		result.clear();

	content = result;
}

void ConfigParser::cleanContent(std::string& content) 
{
	removeComments(content);
	trimWhitespace(content);
	collapseSpaces(content);
}

void ConfigParser::validateFilePath(const std::string& path) 
{
	int fileType = _configFile.getTypePath(path);

	switch (fileType) 
	{
		case 0:
			throw std::runtime_error("Configuration file does not exist: " + path);
		case 2:
			throw std::runtime_error("Path is a directory, not a file: " + path);
		case 3:
			throw std::runtime_error("Path is a special file: " + path);
		case 1:
			break; // Regular file, no action needed
		default:
			throw std::runtime_error("Unknown file type: " + path);
	}

	if (!_configFile.checkFile(path, 0)) 
		throw std::runtime_error("No read permission for file: " + path);
}

void ConfigParser::parseServerBlocks(const std::string& content) 
{
	_servers.clear();

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

	return blockEnd;
}

void ConfigParser::parseServerBlock(const std::string& block) 
{
	ServerConfig server;
	LocationConfig defaultLocation;

	server.addLocation("/", defaultLocation);
	size_t pos = 0;

	while (pos < block.size()) 
	{
		while (pos < block.size() && std::isspace(block[pos])) 
			++pos;

		if (pos >= block.size()) 
			break;

		// Find directive end
		size_t endDirective = block.find_first_of(";\n{", pos);
		if (endDirective == std::string::npos) 
		{
			break;
		}

		std::string directiveLine = Utils::trim(block.substr(pos, endDirective - pos));
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

		std::string key = Utils::trim(directiveLine.substr(0, spacePos));
		std::string value = Utils::trim(directiveLine.substr(spacePos + 1));

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
				server.setClientMaxBodySize(value);
			else if (key == "client_body_temp_path") 
				server.setClientBodyTmpPath(value);
			else if (key == "error_page") 
				server.setErrorPage(value);

			pos = endDirective + 1;
		}
	}

	_servers.push_back(server);  // Add to vector
}

LocationConfig ConfigParser::parseLocationBlock(const std::string& locationHeader, 
						const std::string& block) 
{
	LocationConfig location;
	std::istringstream iss(block);
	std::string line;

	location.setPath(locationHeader);

	while (std::getline(iss, line)) 
	{
		line = Utils::trim(line);
		if (line.empty() || line == "}") 
			continue;

		size_t pos = line.find_first_of(" \t");
		if (pos == std::string::npos) 
			throw std::runtime_error("Invalid config line in location block: " + line);

		std::string key = Utils::trim(line.substr(0, pos));
		std::string value = Utils::trim(line.substr(pos + 1));

		// Remove trailing semicolon or opening brace
		if (!value.empty() && (value[value.size() - 1] == ';' || value[value.size() - 1] == '{')) 
			value = value.substr(0, value.size() - 1);

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
		else if (key == "cgi_extension") 
			location.setCgiExtension(value);
		else if (key == "upload_path") 
			location.setUploadPath(value);
		else if (key == "cgi_path") 
			location.setCgiPath(value);
	}

	return location;
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter) 
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream iss(str);

	while (std::getline(iss, token, delimiter)) 
	{
		token = Utils::trim(token);
		if (!token.empty()) 
			tokens.push_back(token);
	}
	return tokens;
}
