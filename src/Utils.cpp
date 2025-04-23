/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/18 17:17:40 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/19 22:32:45 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Utils.hpp"

std::string Utils::getMimeType(const std::string &path) 
{
	size_t dot_pos = path.find_last_of(".");
	if (dot_pos == std::string::npos)
		return "application/octet-stream";

	std::string extension = path.substr(dot_pos);
	if (extension == ".html" || extension == ".htm") 
		return "text/html";
	if (extension == ".css") 
		return "text/css";
	if (extension == ".js") return 
		"application/javascript";
	if (extension == ".json") 
		return "application/json";
	if (extension == ".png") 
		return "image/png";
	if (extension == ".jpg" || extension == ".jpeg") 
		return "image/jpeg";
	if (extension == ".gif") 
		return "image/gif";
	if (extension == ".svg") 
		return "image/svg+xml";
	if (extension == ".ico") 
		return "image/x-icon";
	if (extension == ".txt") 
		return "text/plain";
	if (extension == ".pdf") 
		return "application/pdf";
	return "application/octet-stream";
}

bool Utils::isPathWithinRoot(const std::string& root, const std::string& path) 
{
	return path.find(root) == 0;
}

bool Utils::isDirectory(const std::string& path)
{
	struct stat pathStat;
	if (stat(path.c_str(), &pathStat) != 0)
		return false;
	return S_ISDIR(pathStat.st_mode);
}

bool Utils::fileExists(const std::string& path) 
{
	struct stat fileInfo;

	if (stat(path.c_str(), &fileInfo))
		return false;
	return S_ISREG(fileInfo.st_mode);
}

std::string Utils::trim(const std::string& str) 
{
	size_t first = str.find_first_not_of(" \t");
	size_t last = str.find_last_not_of(" \t");
	if (first == std::string::npos || last == std::string::npos) 
		return "";
	return str.substr(first, last - first + 1);
}

void Utils::skipWhitespace(std::string& str) 
{
	size_t i = 0;
	while (i < str.size() && (str[i] == ' ' || str[i] == '\t')) 
		i++;
	str.erase(0, i);
}

std::string Utils::listDirectory(const std::string& dirPath, const std::string& root, const std::string& requestUri)
{
	if (!isPathWithinRoot(root, dirPath)) 
		return "<html><body><h1>403 Forbidden</h1></body></html>";

	DIR* dir = opendir(dirPath.c_str());
	if (dir == NULL) 
		return "<html><body><h1>404 Not Found</h1></body></html>";

	struct dirent* entry;
	std::vector<std::string> files;

	while ((entry = readdir(dir)) != NULL) 
	{
		std::string name(entry->d_name);
		if (name != "." && name != "..") 
			files.push_back(name);
	}
	closedir(dir);

	std::sort(files.begin(), files.end());

	std::stringstream body;
	body << "<html><head><title>Index of " << requestUri << "</title></head><body>";
	body << "<h1>Index of " << requestUri << "</h1>";
	body << "<ul>";

	if (requestUri != "/" && requestUri != "") 
	{
		std::string parent = requestUri;

		if (!parent.empty() && parent[parent.size() - 1] == '/')
			parent.erase(parent.size() - 1);

		size_t lastSlash = parent.find_last_of('/');
		if (lastSlash != std::string::npos) 
			parent = parent.substr(0, lastSlash + 1);
		else 
			parent = "/";
		body << "<li><a href=\"" << parent << "\">../</a></li>";
	}
	for (size_t i = 0; i < files.size(); ++i) 
	{
		const std::string& file = files[i];

		std::string fullPath = dirPath + "/" + file;
		struct stat s;
		std::string fileLink = requestUri;
		if (!fileLink.empty() && fileLink[fileLink.size() - 1] != '/')
			fileLink += "/";
		fileLink += file;

		if (stat(fullPath.c_str(), &s) == 0 && S_ISDIR(s.st_mode)) 
			body << "<li><a href=\"" << fileLink << "/\">" << file << "/</a></li>";
		else 
			body << "<li><a href=\"" << fileLink << "\">" << file << "</a></li>";
	}

	body << "</ul></body></html>";
	return body.str();
}

int Utils::urlDecode(std::string& str) 
{
	std::string result;
	for (size_t i = 0; i < str.size(); i++) 
	{
		if (str[i] == '%') 
		{
			if (i + 2 >= str.size() || !isxdigit(str[i+1]) || !isxdigit(str[i+2])) 
				return -1;
			std::string hex = str.substr(i+1, 2);
			char decoded = static_cast<char>(strtol(hex.c_str(), NULL, 16));
			result += decoded;
			i += 2;
		} 
		else if (str[i] == '+') 
			result += ' ';
		else 
			result += str[i];
	}
	str = result;
	return 0;
}


bool strToSizeT(const std::string& str, size_t& result, bool allowZero = false) 
{
	if (str.empty()) 
		return (false);

	for (size_t i = 0; i < str.size(); ++i) 
		if (!isdigit(str[i])) 
			return false;

	std::stringstream ss(str);
	ss >> result;

	if (ss.fail() || !ss.eof()) 
		return (false);

	if (!allowZero && result == 0) 
		return (false);

	return (true);
}
