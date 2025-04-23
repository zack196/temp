/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigFile.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/28 00:24:59 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/09 21:49:17 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ConfigFile.hpp"

ConfigFile::ConfigFile() : _path(""), _size(0)
{
}

ConfigFile::ConfigFile(const std::string& path) : _path(path), _size(0) 
{
	struct stat fileStat;
	if (stat(path.c_str(), &fileStat) == 0) 
		_size = fileStat.st_size;
}

ConfigFile::ConfigFile(const ConfigFile& other)
{
	*this = other;
}

ConfigFile& ConfigFile::operator=(const ConfigFile& other) 
{
	if (this != &other) 
	{
		_path = other._path;
		_size = other._size;
	}
	return *this;
}

ConfigFile::~ConfigFile() 
{
}

int ConfigFile::getTypePath(const std::string &path)
{
	struct stat fileStat;
	if (stat(path.c_str(), &fileStat) != 0) 
		return 0; 
	if (S_ISREG(fileStat.st_mode))
		return 1;
	if (S_ISDIR(fileStat.st_mode))
		return 2;
	return 3;
}

int ConfigFile::checkFile(const std::string& path, int mode)
{
	switch (mode)
	{
		case 0: 
			return access(path.c_str(), R_OK) == 0 ? 1 : 0;
		case 1: 
			return access(path.c_str(), W_OK) == 0 ? 1 : 0;
		case 2: 
			return access(path.c_str(), X_OK) == 0 ? 1 : 0;
		default: 
			return 0;
	}
}

bool ConfigFile::readFile(const std::string& path, std::string& output) 
{
	std::ifstream file(path.c_str());
	if (!file) 
		return false;

	std::ostringstream oss;
	oss << file.rdbuf();
	output = oss.str();

	return
		true;
}

// Check if file exists and is readable. If not, try appending the index string.
bool ConfigFile::isFileExistAndReadable(const std::string& path, const std::string& index)
{
	std::ifstream infile(path.c_str());
	if (infile.good())
		return true;
	std::ifstream infile_index((path + index).c_str());
	return (infile_index.good()) ? 1 : 0;
}

const std::string& ConfigFile::getPath() const
{
	return _path;
}

size_t ConfigFile::getSize() const
{
	return (_size);
}

