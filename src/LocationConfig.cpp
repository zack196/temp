#include "../include/LocationConfig.hpp"
#include "../include/Utils.hpp"
#include <sstream>
#include <iostream>

LocationConfig::LocationConfig() : _root("./www/html"), _path("/"), _index("index.html"), _autoindex(false), _cgiExtension(""), _cgiPath(""), _uploadPath("")
{
	_redirect = std::make_pair(0, "");
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
		_redirect = other._redirect;
		_uploadPath = other._uploadPath;
	}
	return *this;
}

void LocationConfig::setRoot(const std::string& root)
{
	_root = root;
}

void LocationConfig::setPath(const std::string& path)
{
	_path = path;
}

void LocationConfig::setIndex(const std::string& index)
{
	_index = index;
}

void LocationConfig::setAutoindex(const std::string& autoindex) 
{
	if (autoindex == "on") 
		_autoindex = true;
	else if (autoindex == "off") 
		_autoindex = false;
	else 
		throw std::runtime_error("Invalid autoindex value: " + autoindex + " (must be 'on' or 'off')");
}

void LocationConfig::setUploadPath(const std::string& path)
{
	_uploadPath = path;
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

std::string LocationConfig::getResource(const std::string& requestPath) const 
{
	std::string root = _root;
	std::string path = requestPath;

	if (!root.empty() && root[root.length() - 1] != '/') 
		root += '/';

	if (!path.empty() && path[0] == '/') 
		path = path.substr(1);

	std::string full_path = root + path;

	if (Utils::isDirectory(full_path)) 
	{
		std::string index_path = full_path;
		if (!index_path.empty() && index_path[index_path.length() - 1] != '/') 
			index_path += '/';

		index_path += _index;
		if (Utils::fileExists(index_path)) 
			return index_path;

		return full_path;
	}

	if (Utils::fileExists(full_path)) 
		return full_path;

	return "";
}

void LocationConfig::setAllowedMethods(const std::string& methods)
{
	std::vector<std::string> methodsList = split(methods, ' ');
	_allowedMethods.clear();

	for (std::vector<std::string>::iterator it = methodsList.begin(); it != methodsList.end(); ++it)
	{
		std::string method = *it;

		if (method == "GET" || method == "POST" || method == "DELETE") 
			_allowedMethods.push_back(method);
		else 
			throw std::runtime_error("Invalid HTTP method: " + method + " (allowed: GET, POST, DELETE)");
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

void LocationConfig::setRedirect(const std::string& redirectValue) 
{
	std::istringstream iss(redirectValue);
	int code;
	std::string url;

	if (!(iss >> code >> url)) 
		throw std::runtime_error("Invalid redirect format: " + redirectValue + 
			   " (should be 'code url')");

	if (code < 300 || code > 308) 
		throw std::runtime_error("Invalid redirect code: " + Utils::toString(code) + " (must be between 300-308)");

	if (!url.empty() && url[url.length()-1] == ';') 
		url.erase(url.length()-1);

	_redirect = std::make_pair(code, url);
}

const std::string& LocationConfig::getRoot() const 
{
	return _root;
}

const std::string& LocationConfig::getPath() const 
{
	return _path;
}

const std::string& LocationConfig::getIndex() const
{
	return _index;
}

bool LocationConfig::getAutoindex() const
{
	return _autoindex;
}

bool LocationConfig::isAutoIndexOn() const 
{
	return _autoindex;
}

const std::vector<std::string>& LocationConfig::getAllowedMethods() const
{
	return _allowedMethods;
}

const std::pair<int, std::string>& LocationConfig::getRedirect() const 
{
	return _redirect;
}

int LocationConfig::getRedirectCode() const 
{
	return _redirect.first;
}

const std::string& LocationConfig::getRedirectPath() const 
{
	return _redirect.second;
}

const std::string& LocationConfig::getCgiExtension() const
{
	return _cgiExtension; 
}

const std::string& LocationConfig::getCgiPath() const
{
	return _cgiPath;
}

const std::string& LocationConfig::getUploadPath() const
{
	return _uploadPath;
}

bool LocationConfig::isMethodAllowed(const std::string& method) const
{
	if (_allowedMethods.empty())
		return true;

	for (std::vector<std::string>::const_iterator it = _allowedMethods.begin(); 
		it != _allowedMethods.end(); ++it) 
		if (*it == method)
			return true;

	return false;
}

bool LocationConfig::hasRedirection() const 
{
	return (_redirect.first >= 300 && _redirect.first <= 308 && !_redirect.second.empty());
}

bool LocationConfig::hasCgi() const 
{
	return (!_cgiExtension.empty() && !_cgiPath.empty());
}

bool LocationConfig::allowsUploads() const
{
	return !_uploadPath.empty();
}
