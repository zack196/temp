/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/04 17:26:11 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/18 22:45:22 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/LocationConfig.hpp"
#include "../include/Utils.hpp"
#include <iostream>
#include <sstream>

LocationConfig::LocationConfig() : _root(""), _path(""), _index(""), _autoindex(false), _cgiExtension(""), _cgiPath("")
{
}

LocationConfig::~LocationConfig()
{
}

LocationConfig::LocationConfig(const LocationConfig& other) 
{
	*this = other;
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other)
{
	if (this != &other)
	{
		_root = other._root;
		_path = other._path;
		_index = other._index;
		_autoindex = other._autoindex;
		_allowedMethods = other._allowedMethods;
		_cgiExtension = other._cgiExtension;
		_cgiPath = other._cgiPath;
	}
	return *this;
}

void LocationConfig::setRoot(const std::string& root)
{
	_root = root; 
}

void LocationConfig::setIndex(const std::string& index)
{
	_index = index;
}

void LocationConfig::setAutoindex(const std::string& autoindex) 
{
	if (autoindex == "on" || autoindex== "off")
		this->_autoindex = (autoindex == "on");
	else
		throw std::runtime_error("Wrong autoindex");
}

bool LocationConfig::isAutoIndexOn() const 
{
	return (_autoindex);
}


std::vector<std::string> LocationConfig::split(const std::string& str, char delimiter) 
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

std::string LocationConfig::resolvePath(const std::string& requestPath) const 
{
	size_t prefixPos = requestPath.find(_path);
	if (prefixPos == 0)
	{
		std::string relativePath = requestPath.substr(_path.length());
		return _root + relativePath;
	}
	return (_root + requestPath);
}

void LocationConfig::setAllowedMethods(const std::string& methods)
{
	std::vector<std::string> methodsList = split(methods, ' ');

	for (std::vector<std::string>::iterator it = methodsList.begin(); it != methodsList.end(); ++it)
	{
		std::string method = *it;

		if (method == "GET" || method == "POST" || method == "DELETE" || method == "PUT" || method == "HEAD") 
			_allowedMethods.push_back(method);
		else 
			throw std::runtime_error("Invalid HTTP method: " + method);
	}
}

void LocationConfig::setCgiExtension(const std::string& extension)
{ 
	_cgiExtension = extension;
}

void LocationConfig::setCgiPath(const std::string& path)
{
	_cgiPath = path;
}

const std::string& LocationConfig::getRoot() const 
{
	return _root;
}

const std::string& LocationConfig::getIndex() const
{
	return _index;
}

bool LocationConfig::getAutoindex() const
{
	return _autoindex;
}

void LocationConfig::setPath(const std::string& path)
{
	_path = path;
}

const std::string& LocationConfig::getPath() const 
{
	return _path;
}
const std::vector<std::string>& LocationConfig::getAllowedMethods() const
{
	return _allowedMethods;
}

bool LocationConfig::isMethodAllowed(const std::string& method) const
{
	if (_allowedMethods.empty())
		return true;

	for (std::vector<std::string>::const_iterator it = _allowedMethods.begin(); it != _allowedMethods.end(); ++it) 
		if (*it == method)
			return (true);
	return (false);
}

const std::string& LocationConfig::getCgiExtension() const
{
	return _cgiExtension; 
}

const std::string& LocationConfig::getCgiPath() const
{
	return _cgiPath;
}

void LocationConfig::print() const
{
	std::cout << "  Location Config:\n";
	std::cout << "    Root: " << _root << "\n";
	std::cout << "    Path: " << _path << "\n";
	std::cout << "    Index: " << _index << "\n";
	std::cout << "    Autoindex: " << (_autoindex ? "on" : "off") << "\n";
	std::cout << "    Allowed Methods: ";
	for (size_t i = 0; i < _allowedMethods.size(); ++i)
		std::cout << _allowedMethods[i] << " ";
	std::cout << "\n";
	std::cout << "    CGI Extension: " << _cgiExtension << "\n";
	std::cout << "    CGI Path: " << _cgiPath << "\n";
}

