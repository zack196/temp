#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <cstddef>
#include <ostream>
#include <string>
#include <map>
#include <fstream>
#include "Utils.hpp"
#include "LocationConfig.hpp"
#include <iostream>


class ServerConfig;

class HTTPRequest {
public:
	enum ParseState {
		INIT,
		LINE_METHOD,
		LINE_URI,
		LINE_VERSION,
		HEADER_KEY,
		HEADER_VALUE,
		BODY_INIT,
		BODY_CHUNKED,
		BODY_MULTIPART,
                BODY_CGI,
		FINISH,
		ERRORE
	};

	enum MultipartState {
		PART_HEADER,
		PART_DATA,
		PART_BOUNDARY,
		PART_END
	};

	enum ChunkState {
		CHUNK_SIZE,
		CHUNK_DATA,
		CHUNK_FINISHED
	};	HTTPRequest(ServerConfig* server);
	HTTPRequest(const HTTPRequest& other);
	HTTPRequest& operator=(const HTTPRequest& other);
	~HTTPRequest();

	bool keepAlive() const;
	const std::map<std::string, std::string>& getHeaders() const;
	bool isComplete() const;
	bool hasCgi();
	const std::string& getUri() const;
	const std::string& getProtocol() const;
	const std::string& getQuery() const;
	const std::string& getBody() const;
	const std::string& getMethod() const;
	const std::string& getResource() const;
	const std::string& getPath() const;
        const std::string& getBodyfile() const;
	std::string getQueryParameter(const std::string& key) const;
	int getState() const;
	int getStatusCode() const;
	size_t getContentLength() const;
	const std::string& getHeader(const std::string& key) const;

	ServerConfig* getServer() const;

	void setStatusCode(int code);
	void setState(ParseState state);
	void parse(std::string& rawdata);
	void clear();
	const LocationConfig* getLocation() const;
	bool isInternalCGIRequest() const;

	void print()
	{
		std::cout << "method= " << _method << std::endl;
		std::cout << "path= " << _path << std::endl;
		std::cout << "protocol= " << _protocol << std::endl;
		for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); it++)
			std::cout << it->first << ": " << it->second << std::endl;
		std::cout << "body: \n";
		std::cout << _bodyBuffer << std::endl;
	}
	void setUsername(const std::string& username);
	const std::string& getSessionId() const ;
    const std::string& getUsername()  const ;

protected:
	void parseMethod(std::string& rawdata);
	void parseUri(std::string& rawdata);
	void parseQueryParameters(const std::string& query);
	void parseVersion(std::string& rawdata);
	void parseHeadersKey(std::string& rawdata);
	void parseHeadersValue(std::string& rawdata);
	void parseRequestBody(std::string& rawdata);

	void parseChunkBody(std::string& rawdata);
	bool processChunkSize(std::string& rawdata);
	bool processChunkData(std::string& rawdata);

	void parseMultipartBody(std::string& rawdata);
	bool processPartHeader(std::string& rawdata);
	bool processPartData(std::string& rawdata);
	bool processPartBoundary(std::string& rawdata);
        void writeBodyToFile(std::string& rawdata);

	bool validateHostHeader();
	bool validateContentLength();
	bool validateTransferEncoding();
	bool validateMultipartFormData();
	bool validateAllowedMethods();
	bool validateCgi();

	void findLocation(const std::string& path);
	void closeOpenFiles();
	

	const std::string& getCookie(const std::string& name) const;
	void parseCookieHeader(const std::string& line);
	
    bool isAuthenticated() const ;

private:
	HTTPRequest();

	ServerConfig* _server;
	int           _statusCode;
	ParseState    _state;
	size_t        _parsePosition;
	size_t        _contentLength;
	std::string   _method;
	std::string   _uri;
	std::string   _path;
	std::string   _query;
	std::string   _protocol;
	std::string   _bodyBuffer;
	std::string   _headerKey;
	std::string   _headerValue;
	std::map<std::string, std::string> _headers;
	std::map<std::string, std::string> _queryParameters;
	std::string   _contentType;
	std::string   _boundary;
	std::string   _resource;
	bool          _isChunked;
	bool          _keepAlive;
        size_t           _chunkSize;
	size_t        _receivedLength;
	std::string   _multipartBoundary;
	const LocationConfig* _location;
	bool          _uploadHeadersParsed;
	bool          _chunkFileInitialized;
	std::string   _uploadFilepath;
	std::string   _uploadBoundary;
	MultipartState _multipartState;
	ChunkState     _chunkState;
	bool           _hasCgi;
	std::string    _chunkFilePath;
	std::ofstream  _uploadFile;
	std::ofstream  _chunkFile;
    std::ofstream   _bodyStream; 
    std::string   _bodyTempFile;

	std::map<std::string, std::string> _cookies;
	std::string	_sessionId;
	std::string	_username;

};

#endif
