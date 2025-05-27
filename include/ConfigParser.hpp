#pragma once
#include <string>
#include <vector>
#include "ServerConfig.hpp"
#include "FileHandler.hpp"

class ConfigParser {
public:
    ConfigParser();
    explicit ConfigParser(const std::string& configFilePath);
    ConfigParser(const ConfigParser& other);
    ConfigParser& operator=(const ConfigParser& other);

    void parseFile();
    std::vector<ServerConfig>& getServers() { return _servers; }
    const std::vector<ServerConfig>& getServers() const { return _servers; }

private:
    void removeComments(std::string& content);
    void trimWhitespace(std::string& content);
    void collapseSpaces(std::string& content);
    void cleanContent(std::string& content);
    void validateFilePath(const std::string& path);
    void parseServerBlocks(const std::string& content);
    size_t findBlockStart(const std::string& content, size_t pos);
    size_t findBlockEnd(const std::string& content, size_t blockStart);
    void parseServerBlock(const std::string& block);
    LocationConfig parseLocationBlock(const std::string& locationHeader, const std::string& block);
    std::vector<std::string> split(const std::string& str, char delimiter);

    FileHandler _configFile;
    std::vector<ServerConfig> _servers;  // Store ServerConfig objects directly, not pointers
};
