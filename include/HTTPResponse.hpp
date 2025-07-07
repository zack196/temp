#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <fstream>
#include "HTTPRequest.hpp"
#include "CGIHandler.hpp"

class HTTPResponse 
{
public:
	HTTPResponse(HTTPRequest* request);
	~HTTPResponse();

	bool isComplete();
	bool isReady();
	void clear();

	ssize_t getResponseChunk(char* buffer);
	ssize_t getHeaderChunk(char* buffer);
	ssize_t getContentChunk(char* buffer);

	CGIHandler& getCgiHandler();

	void buildResponse();
	void buildErrorResponse(int statusCode);
	void buildSuccessResponse(const std::string& fullPath);

	bool isCgiRuning();
	void startCgi();
	void buildCgiResponse();
	void readCgiFileAndParse(const std::string& filepath);
	void handleGet();
	void handlePost();
	void handleDelete();
	void handleRedirect();
	void handleAutoIndex();

	void setProtocol(const std::string& protocol);
	void setStatusCode(int code);
	void setStatusMessage(const std::string& message);
	void setHeader(const std::string& key, const std::string& value);
	void setBodyResponse(const std::string& body);

	std::string getHeader() const;
	std::string getBody() const;
	std::string getFilePath() const;
	size_t      getContentLength() const;
	size_t      getFileSize() const;

	bool shouldKeepAlive() const;
	void buildHeader();

	void parseCgiResponse(const std::string& chunk);
	bool parseHeaders(const std::string& headers);
	void writeToFile(const std::string& data);


	void printRespons();

private:
	HTTPResponse();
	HTTPResponse(const HTTPResponse&);
	HTTPResponse& operator=(const HTTPResponse&);

	
	HTTPRequest*             _request;
	std::string              _protocol;
	int                      _statusCode;
	std::string              _statusMessage;
	std::map<std::string,std::string> _headers;
	std::string              _body;
	std::string              _header;
	std::string              _filePath;
	size_t                   _fileSize;
	CGIHandler               _cgiHandler;
	bool                     _cgiRuning;
	bool                     _hasCgiOutput;
	bool                     _cgiHeaderComplete;
	std::string              _cgiFile;
	std::ifstream 		 _fileStream;
	std::ofstream            _cgiOutput;
	size_t                  _bytesSent;
	bool                     _headerSent;
	bool                     _isComplete;
	bool                     _isReady;

};

#endif // HTTPRESPONSE_HPP

