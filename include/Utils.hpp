/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/18 17:22:42 by mregrag           #+#    #+#             */
/*   Updated: 2025/05/08 19:30:02 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <map>
#include <sys/stat.h>
#include <dirent.h>
#include <sstream>
#include <vector>
#include <algorithm>

namespace Utils 
{
	template <typename T>
	std::string toString(const T& value)
	{
		std::ostringstream oss;
		oss << value;
		return oss.str();
	}	

	std::string getMimeType(const std::string &path);
	bool isDirectory(const std::string &path);
	std::string generateAutoIndex(const std::string &path, const std::string &requestUri);
	std::string listDirectory(const std::string& dirPath, const std::string& root, const std::string& requestUri);
	std::string getMessage(int code);
	bool isPathWithinRoot(const std::string& root, const std::string& path);
	bool isDirectory(const std::string& path);
	bool fileExists(const std::string& path);
	std::string trim(const std::string& str);
	void skipWhitespace(std::string& str);
	int urlDecode(std::string& str);
	size_t stringToSizeT(const std::string& str);
	bool isValidMethodToken(const std::string& method);
	std::string getCurrentDate();
	std::string getCurrentDate(size_t delta_t);

	size_t skipLeadingWhitespace(const std::string& str);
	
	bool isValidMethodChar(char c);

	bool isValidUri(const std::string& uri);
	bool isSupportedMethod(const std::string& method);

	bool isValidVersion(const std::string& version);
	bool isValidVersionChar(char c);

	bool isValidHeaderKey(const std::string& key);

	bool isValidHeaderValue(const std::string& value);
	bool isValidHeaderValueChar(char c);

	bool isValidHeaderKeyChar(char c);
	bool isValidHeaderValueChar(char c);

	std::string extractAttribute(const std::string& headers, const std::string& key);
	std::vector<std::string> listDirectory(const std::string& path);
	std::string readFileContent(const std::string& filePath);
	std::string getFileExtension(const std::string& path);
	bool isExecutable(const std::string& path);
	char** mapToEnvp(const std::map<std::string, std::string>& env);
	void freeEnvp(char** envp);
	std::string createTempFile(const std::string& prefix, const std::string& dir);

	// simple helper in Utils.hpp
	struct CookieAttr
	{
		std::string path, domain, sameSite, expires;
		int         maxAge;      // -1 => omit
		bool        secure, httpOnly;
		CookieAttr();
	};
	std::string buildCookieAttributes(const CookieAttr& a);
	
	std::map<std::string,std::string> parseUrlEncoded(const std::string& body);


}


#endif

