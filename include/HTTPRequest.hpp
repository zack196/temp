#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

class HTTPRequest
{
public:
	enum ParseState
	{
		INIT,
		METHOD,
		URI,
		PROTOCOL,
		HEADER,
		BODY_INIT,
		CGI,
		CHUNKED,
		MULTIPART,
		FINISH,
		ERROR
	};

	enum MultipartState
	{
		PART_HEADER,
		PART_DATA,
		PART_BOUNDARY,
		PART_END
	};

	enum ChunkState
	{
		CHUNK_SIZE,
		CHUNK_DATA
	};

private:
	ServerConfig                        _server;
	std::vector<ServerConfig>           _servers;
	LocationConfig                      _location;
	int                                 _statusCode;
	ParseState                          _state;
	size_t                              _parsePosition;
	size_t                              _contentLength;
	std::string                         _method;
	std::string                         _uri;
	std::string                         _path;
	std::string                         _query;
	std::string                         _protocol;
	std::string                         _bodyBuffer;
	std::map<std::string, std::string>  _headers;
	std::map<std::string, std::string>  _queryParameters;
	std::string                         _contentType;
	std::string                         _resource;
	bool                                _isChunked;
	bool                                _keepAlive;
	int                                 _chunkSize;
	size_t                              _length;
	std::string                         _boundary;
	MultipartState                      _multipartState;
	ChunkState                          _chunkState;
	std::string                         _bodyFile;
	std::ofstream                       _body;
	std::ofstream                       _uploadFile;
	size_t                              _totalBodySize;
	int                                 _client_fd;

public:
	HTTPRequest(std::vector<ServerConfig>& servers);
	~HTTPRequest();

	void parseRequest(std::string& data);
	bool isComplete() const;
	bool keepAlive() const;
	int getState() const;
	int getStatusCode() const;
	void setStatusCode(int code);
	void setState(ParseState state);

	const std::string& getMethod() const;
	const std::string& getUri() const;
	const std::string& getPath() const;
	const std::string& getQuery() const;
	const std::string& getProtocol() const;
	const std::string& getBody() const;
	const std::string& getResource() const;
	const std::string& getBodyfile() const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getHeader(const std::string& key) const;
	size_t getContentLength() const;
	const ServerConfig& getServer() const;
	const LocationConfig& getLocation() const;
	std::string getQueryParameter(const std::string& key) const;

	void setClientfd(int fd);

	bool hasCgi();
	void clear();
	std::string getSocketIp(int fd);
	const ServerConfig& findServerByHost(const std::string& value);

private:
	void parseMethod(std::string& data);
	void parseUri(std::string& data);
	void parseProtocol(std::string& data);
	void parseHeaders(std::string& data);
	void parseBody();
	void parseChunkBody(std::string& data);
	void parseMultipartBody(std::string& data);

	bool validateHostHeader();
	bool validateContentLength();
	bool validateTransferEncoding();
	bool validateMultipartFormData();
	bool validateAllowedMethods();
	bool validateCgi();

	bool readChunkSize(std::string& data);
	bool readChunkData(std::string& data);

	bool processPartHeader(std::string& data);
	bool processPartData(std::string& data);
	bool processPartBoundary(std::string& data);

	void writeBodyToFile(std::string& data);
};

#endif // HTTPREQUEST_HPP
