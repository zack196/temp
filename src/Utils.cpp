/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/18 17:17:40 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/22 00:43:17 by mregrag          ###   ########.fr       */
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

std::string Utils::getMessage(int code) 
{
	switch(code) 
	{
		// Success
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No Content";
		case 205: return "Reset Content";
		case 206: return "Partial Content";
		case 207: return "Multi-Status";
		case 208: return "Already Reported";
		case 226: return "IM Used";

			  // Redirection
		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";
		case 305: return "Use Proxy";
		case 306: return "Switch Proxy";
		case 307: return "Temporary Redirect";
		case 308: return "Permanent Redirect";

			  // Client Errors
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 406: return "Not Acceptable";
		case 407: return "Proxy Authentication Required";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 410: return "Gone";
		case 411: return "Length Required";
		case 412: return "Precondition Failed";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 416: return "Range Not Satisfiable";
		case 417: return "Expectation Failed";
		case 418: return "I'm a teapot";
		case 421: return "Misdirected Request";
		case 422: return "Unprocessable Entity";
		case 423: return "Locked";
		case 424: return "Failed Dependency";
		case 425: return "Too Early";
		case 426: return "Upgrade Required";
		case 428: return "Precondition Required";
		case 429: return "Too Many Requests";
		case 431: return "Request Header Fields Too Large";
		case 451: return "Unavailable For Legal Reasons";

			  // Server Errors
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		case 505: return "HTTP Version Not Supported";
		case 506: return "Variant Also Negotiates";
		case 507: return "Insufficient Storage";
		case 508: return "Loop Detected";
		case 510: return "Not Extended";
		case 511: return "Network Authentication Required";

		default: return "Unknown Status";
	}
}

size_t Utils::stringToSizeT(const std::string& str) 
{
	std::istringstream iss(str);
	size_t result;
	iss >> result;

	if (iss.fail()) 
		throw std::invalid_argument("Invalid size_t conversion: " + str);

	char remaining;
	if (iss >> remaining) 
		throw std::invalid_argument("Extra characters after number: " + str);
	return (result);
}
