#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <vector>
#include <utility> // For std::pair
#include <stdexcept>

class LocationConfig 
{
private:
	std::string _root;                      // Root directory for this location
	std::string _path;                      // URL path for this location
	std::string _index;                     // Default index file
	bool _autoindex;                        // Directory listing enabled/disabled
	std::vector<std::string> _allowedMethods; // Allowed HTTP methods
	std::string _cgiExtension;              // File extension for CGI scripts
	std::string _cgiPath;                   // Path to CGI interpreter
	std::pair<int, std::string> _redirect;  // Redirection code and URL
	std::string _uploadPath;                // Path for file uploads

	std::vector<std::string> split(const std::string& str, char delimiter);

public:
	LocationConfig();
	LocationConfig(const LocationConfig& other);
	~LocationConfig();
	LocationConfig& operator=(const LocationConfig& other);

	void setRoot(const std::string& root);
	void setPath(const std::string& path);
	void setIndex(const std::string& index);
	void setAutoindex(const std::string& autoindex);
	void setAllowedMethods(const std::string& methods);
	void setCgiExtension(const std::string& extension);
	void setCgiPath(const std::string& path);
	void setRedirect(const std::string& redirectValue);
	void setUploadPath(const std::string& path);

	const std::string& getRoot() const;
	const std::string& getPath() const;
	const std::string& getIndex() const;
	bool getAutoindex() const;
	const std::vector<std::string>& getAllowedMethods() const;
	const std::string& getCgiExtension() const;
	const std::string& getCgiPath() const;
	const std::pair<int, std::string>& getRedirect() const;
	int getRedirectCode() const;
	const std::string& getRedirectPath() const;
	const std::string& getUploadPath() const;
	bool isAutoIndexOn() const;

	bool isMethodAllowed(const std::string& method) const;

	bool hasRedirection() const;

	bool hasCgi() const;

	bool allowsUploads() const;

	std::string getResource(const std::string& requestPath) const;
};

#endif // LOCATION_CONFIG_HPP
