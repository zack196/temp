/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/04 17:16:39 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/20 22:49:05 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserver.hpp"

#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <utility>
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include "ConfigFile.hpp"

class ConfigParser 
{
	public:
		// Constructors and Destructor
		ConfigParser();
		ConfigParser(const std::string& configFilePath);
		ConfigParser(const ConfigParser& other);
		ConfigParser& operator=(const ConfigParser& other);
		~ConfigParser();

		void parseFile();
		const std::vector<ServerConfig>& getServers();
		void print() const;

	private:
		ConfigFile _configFile;
		std::vector<ServerConfig> _servers;

		void cleanContent(std::string& content);

		void parseServerBlock(const std::string& block);
		LocationConfig parseLocationBlock(const std::string& locationHeader, const std::string& block);

		std::pair<std::string, std::string> parseLine(const std::string& line);

		std::vector<std::string> split(const std::string& str, char delimiter);

		// Utility functions as member functions
		void parseServerBlocks(const std::string& content);
		void removeComments(std::string& content);
		void trimWhitespace(std::string& content);
		void collapseSpaces(std::string& content);
		size_t findBlockStart(const std::string& content, size_t pos);
		size_t findBlockEnd(const std::string& content, size_t blockStart);
		void validateFilePath(const std::string& path);
		std::string readFileContent(const std::string& path);
};

#endif
