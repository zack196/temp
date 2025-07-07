#include "../include/LocationConfig.hpp"
#include "../include/Utils.hpp"
#include <sstream>
#include <iostream>

LocationConfig::LocationConfig() : _root("./www/html"), _path("/"), _index("index.html"), _autoindex(false), _uploadPath("./www/upload"), _redirectCode(0), _redirectPath("")
{
}

LocationConfig::~LocationConfig()
{
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


std::string LocationConfig::getResource(const std::string& requestPath) const 
{

	std::string relativePath = requestPath;

	if (relativePath.find(_path) == 0)
		relativePath.erase(0, _path.length());

	if (!relativePath.empty() && relativePath[0] == '/')
		relativePath.erase(0, 1);

	std::string rootPath = _root;
	if (!rootPath.empty() && rootPath[rootPath.length() - 1] != '/')
		rootPath += '/';

	std::string fullPath = rootPath + relativePath;

	if (Utils::isDirectory(fullPath)) 
	{
		if (fullPath[fullPath.length() - 1] != '/')
			fullPath += '/';

		std::string indexPath = fullPath + _index;
		if (Utils::isFileExists(indexPath))
			return indexPath;

		return fullPath;
	}
	if (Utils::isFileExists(fullPath)) 
		return fullPath;
	return "";
}



void LocationConfig::setAllowedMethods(const std::string& methods)
{
	std::vector<std::string> methodsList = Utils::split(methods, ' ');

	for (std::vector<std::string>::iterator it = methodsList.begin(); it != methodsList.end(); ++it)
	{
		std::string method = *it;

		if (method == "GET" || method == "POST" || method == "DELETE") 
			_allowedMethods.push_back(method);
		else 
			throw std::runtime_error("Invalid HTTP method: " + method + " (allowed: GET, POST, DELETE)");
	}
}

void LocationConfig::setCgiPath(const std::string& cgiLine)
{
	std::size_t spacePos = cgiLine.find_first_of(" \t");

	if (spacePos == std::string::npos)
		throw std::runtime_error("Invalid cgi_path directive: " + cgiLine);

	std::string ext = Utils::trim(cgiLine.substr(0, spacePos));
	std::string path = Utils::trim(cgiLine.substr(spacePos + 1));

	if (ext.empty() || path.empty())
		throw std::runtime_error("Invalid cgi_path directive: " + cgiLine);
	_cgiPath[ext] = path;
}


void LocationConfig::setRedirect(const std::string& redirectValue) 
{
	std::istringstream iss(redirectValue);
	int code;
	std::string path;

	if (!(iss >> code >> path)) 
		throw std::runtime_error("Invalid redirect format: " + redirectValue + " (should be 'code path')");

	if (code < 300 || code > 308) 
		throw std::runtime_error("Invalid redirect code: " + Utils::toString(code) + " (must be between 300-308)");


	_redirectCode = code;
	_redirectPath = path;
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

int LocationConfig::getRedirectCode() const {
    return _redirectCode;
}

const std::string& LocationConfig::getRedirectPath() const {
    return _redirectPath;
}

std::string LocationConfig::getCgiPath(const std::string& ext) const 
{
	std::map<std::string, std::string>::const_iterator it = _cgiPath.find(ext);
	if (it != _cgiPath.end())
		return it->second;
	return "";
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
	return (!_redirectPath.empty() && _redirectCode != 0);
}

bool LocationConfig::hasCgi() const 
{
	return (!_cgiPath.empty());
}
