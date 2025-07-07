#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <map>
#include <sys/stat.h>
#include <dirent.h>
#include <sstream>
#include <vector>
#include <stdlib.h>

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
	bool isFileExists(const std::string& path);
	bool isFileReadble(const std::string& path);
	std::string trim(const std::string& str);
	void skipWhitespace(std::string& str);
	int urlDecode(std::string& str);
	size_t stringToSizeT(const std::string& str);
	bool isValidMethodToken(const std::string& method);
	std::string getCurrentDate();

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
	std::vector<std::string> split(const std::string& str, char delimiter);
	std::string trimWhitespace(const std::string& str);
	ssize_t parseHexChunk(const std::string& hexstr);
	std::string getExtension(const std::string& path);
	std::string createUploadFile(const std::string& prefix, const std::string& dir);
	bool isFileWritable(const std::string& path);
}


#endif

