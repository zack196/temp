#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "Utils.hpp"
#include "HTTPRequest.hpp"
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <stdexcept>

// Define the available response states.

class CGIHandler;
class HTTPResponse {
public:
	enum response_state {
		INIT,
		HEADER_SENT,
		FINISH,
		ERRORE
	};
	// Constructor now takes an HTTPRequest pointer.
	HTTPResponse(HTTPRequest* request);
	HTTPResponse(const HTTPResponse& rhs);
	HTTPResponse& operator=(const HTTPResponse& rhs);
	~HTTPResponse();

	// Reset the response state.
	void clear();

	// Setter functions.
	void setProtocol(const std::string& protocol);
	void setStatusCode(int code);
	void setStatusMessage(const std::string& message);
	void setHeader(const std::string& key, const std::string& value);
	// Append to the response body.
	void appendToBody(const std::string& bodyPart);
	// Set a file as the response body.
	void setBodyResponse(const std::string& filePath);
	void setState(response_state state);

	// Getter functions.
	std::string getHeader() const;
	std::string getBody() const;
	std::string getFilePath() const;
	size_t getContentLength() const;
	size_t getFileSize() const;
	int getState() const;

	// Logic: should the connection close after this response.
	bool shouldCloseConnection() const;

	// Build the full HTTP header string.
	void buildHeader();

	// Build the response (dispatch to method-specific handlers).
	void buildResponse();

	// Handlers for specific response types/methods.
	void buildErrorResponse(int statusCode);
	void buildSuccessResponse(const std::string& fullPath);
	void handleGet();
	void handlePost();
	void handleDelete();
	void handleRedirect();
	void handleAutoIndex();
	void handleCgi();

	void appendToCgiOutput(const std::string& chunk);
	void finalizeCgiResponse();
	bool hasCgiOutput() const { return _hasCgiOutput; }

	void parseCgiOutput(const std::string& chunk);
	void createCgiTempFile();
	void ensureTempDirectoryExists();
	void writeToFile(const std::string& data);
	bool parseHeaders(const std::string& headers);
	void closeCgiTempFile();

	void addCookie(const std::string& name, const std::string& value, const Utils::CookieAttr& attr = Utils::CookieAttr());
	void handleLogin();
private:
	HTTPRequest* _request; // Directly provided request pointer.
	// The client can be retrieved via _request->getClient() when needed.
	std::string _protocol;
	int _statusCode;
	std::string _statusMessage;
	std::map<std::string, std::string> _headers;
	std::string _body;
	std::string _header; // Final composed header.
	std::string _filePath;
	size_t _fileSize;
	response_state _state;

	std::string _cgiTempFile;
	std::ofstream _cgiStream;
	bool _cgiHeaderComplete;
	bool _hasCgiOutput;

	std::vector<std::string> _setCookies;

};

#endif // HTTPRESPONSE_HPP
