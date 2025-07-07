#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include "Utils.hpp"

class LocationConfig 
{
private:
	std::string _root;
	std::string _path;
	std::string _index;
	bool _autoindex;
	std::vector<std::string> _allowedMethods;
	std::map<std::string, std::string> _cgiPath;
	std::string _uploadPath;
	int _redirectCode;
	std::string _redirectPath;

public:
	LocationConfig();
	~LocationConfig();

	void setRoot(const std::string& root);
	void setPath(const std::string& path);
	void setIndex(const std::string& index);
	void setAutoindex(const std::string& autoindex);
	void setAllowedMethods(const std::string& methods);
	void setUploadPath(const std::string& path);
	void setCgiPath(const std::string& cgiLine);
	void setRedirect(const std::string& redirectValue);

	const std::string& getRoot() const;
	const std::string& getPath() const;
	const std::string& getIndex() const;
	bool getAutoindex() const;
	bool isAutoIndexOn() const;
	const std::vector<std::string>& getAllowedMethods() const;
	int getRedirectCode() const;
	const std::string& getRedirectPath() const;
	std::string getCgiPath(const std::string& ext) const;
	const std::string& getUploadPath() const;

	bool isMethodAllowed(const std::string& method) const;
	bool hasRedirection() const;
	bool hasCgi() const;
	std::string getResource(const std::string& requestPath) const;
};

#endif
