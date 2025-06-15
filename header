#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <map>
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"

class CGIHandler {
public:
	CGIHandler(HTTPRequest* request, HTTPResponse* response);
	~CGIHandler();

	void    setEnv();
	void    start();
	void    cleanup();
	int     getPipeFd() const;
	pid_t   getPid() const;

private:
	void    buildEnv();
	void    buildArgv();
	void    cleanEnv();
	void    cleanArgv();
	void    validatePaths() const;

	pid_t   _pid;
	HTTPRequest*     _request;
	HTTPResponse*    _response;
	std::string     _execPath;
	std::string     _scriptPath;
	std::map<std::string, std::string> _env;
	char**  _envp;
	char**  _argv;
	int     _pipeFd[2];
	time_t  _startTime;
	int     _timeout;
	bool    _pipeClosed;
};

#endif // CGI_HANDLER_HPP
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "HTTPResponse.hpp"
#include <string>
#include <ctime>
#include <netinet/in.h>
#include <sys/types.h>
#include "ServerConfig.hpp"
#include "HTTPRequest.hpp"
#include "SessionManager.hpp"
#include <cstring>

enum ClientState 
{
	CLIENT_READING,
	CLIENT_WRITING,
	CLIENT_ERROR
};

class HTTPRequest;
class HTTPResponse;
class CGIHandler;

class Client
{
private:
    int _fd;                            // File descriptor for the client connection
    ServerConfig* _server;              // Associated server configuration
    HTTPRequest _request;               // HTTP request object
    HTTPResponse _response;             // HTTP response object
    CGIHandler* _cgiHandler;            // CGI handler object
    bool _isCgi;                        // Indicates whether this client is handling a CGI request
    size_t _bytesSent;                  // Bytes sent to the client
    size_t _headersSize;                // Size of the headers sent to the client
    time_t _lastActivity;               // Last activity timestamptime_t
    time_t _cgiStartTime;
    std::string _readBuffer;            // Buffer for reading data from the client
    std::ifstream _fileStream;          // File stream for serving file responses
    Session  *_session;

public:
    Client(int fd, ServerConfig* server);
    ~Client();

    std::string& getReadBuffer();
    void setReadBuffer(const char* data, ssize_t bytesSet);

    void setClientAddress(const struct sockaddr_in& addr);

    ssize_t getResponseChunk(char* buffer, size_t bufferSize);
    ssize_t getHeaderResponse(char* buffer);
    ssize_t getStringBodyResponse(char* buffer);
    ssize_t getFileBodyResponse(char* buffer, size_t bufferSize);

    int getFd() const;
    time_t getLastActivity() const;
    void updateActivity();

    ClientState getState() const;

    HTTPRequest* getRequest();
    HTTPResponse* getResponse();
    ServerConfig* getServer() const;

    bool shouldKeepAlive() const;
    bool hasTimedOut(time_t currentTime, time_t timeout) const;

    void setCgiStartTime(time_t t) { _cgiStartTime = t; }
    time_t getCgiStartTime() const { return _cgiStartTime; }
    void reset();

    // CGI-specific methods
    void setCgiHandler(CGIHandler* cgiHandler);
    CGIHandler* getCgiHandler() const;
    void setIsCgi(bool isCgi);
    bool isCgi() const;

    // session:
    void setSession(Session* s);
    Session*    getSession() const;
};

#endif // CLIENT_HPP
#pragma once
#include <string>
#include <vector>
#include "ServerConfig.hpp"
#include "FileHandler.hpp"

class ConfigParser {
public:
    ConfigParser();
    explicit ConfigParser(const std::string& configFilePath);
    ConfigParser(const ConfigParser& other);
    ConfigParser& operator=(const ConfigParser& other);

    void parseFile();
    std::vector<ServerConfig>& getServers() { return _servers; }
    const std::vector<ServerConfig>& getServers() const { return _servers; }

private:
    void removeComments(std::string& content);
    void trimWhitespace(std::string& content);
    void collapseSpaces(std::string& content);
    void cleanContent(std::string& content);
    void validateFilePath(const std::string& path);
    void parseServerBlocks(const std::string& content);
    size_t findBlockStart(const std::string& content, size_t pos);
    size_t findBlockEnd(const std::string& content, size_t blockStart);
    void parseServerBlock(const std::string& block);
    LocationConfig parseLocationBlock(const std::string& locationHeader, const std::string& block);
    std::vector<std::string> split(const std::string& str, char delimiter);

    FileHandler _configFile;
    std::vector<ServerConfig> _servers;  // Store ServerConfig objects directly, not pointers
};
#ifndef COOKIE_HPP
#define COOKIE_HPP

#include <iostream>
#include <string>
#include <sstream>


class Cookie
{
    private:
    public :
        std::string     _name;
        std::string     _value;
        std::string     _path;
        std::string     _domain;
        time_t          _expires;
        bool            _secure;
        bool            _httpOnly;
        std::string     _sameSite;
        
        Cookie(const std::string& name, const std::string& value);
        std::string toSetCookieString() const;
};


#endif#ifndef EPOLLMANAGER_HPP
#define EPOLLMANAGER_HPP

#include <sys/epoll.h>
#include <vector>

class EpollManager 
{
public:
	EpollManager(size_t maxEvents);
	~EpollManager();

	EpollManager(const EpollManager&);
	EpollManager& operator=(const EpollManager&);

	bool add(int fd, uint32_t events);
	bool modify(int fd, uint32_t events);
	bool remove(int fd);
	int wait(int timeout = -1);
	const epoll_event& getEvent(size_t index) const;
	int getFd() const;

private:
	int _epollFd;
	std::vector<epoll_event> _events;
};

#endif
#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

#include <string>

/**
 * Class for handling file operations in C++98 compatible way
 * Provides utilities for checking file existence, type, and permissions
 */
class FileHandler 
{
private:
	std::string _filePath;

public:
	/* Constructors & Destructor */
	FileHandler();
	FileHandler(const std::string& filePath);
	FileHandler(const FileHandler& other);
	FileHandler& operator=(const FileHandler& other);
	~FileHandler();

	/* Getters & Setters */
	std::string getPath() const;
	void setPath(const std::string& filePath);

	/* File Operations */
	/**
     * Get the type of the path:
     * 0 - does not exist
     * 1 - regular file
     * 2 - directory
     * 3 - special file (device, pipe, socket, etc.)
     */
	int getTypePath(const std::string& path) const;

	/**
     * Check if a file has specific permissions
     * @param path File path to check
     * @param mode 0 for readable, 1 for writable, 2 for executable
     * @return true if the file has the requested permission
     */
	bool checkFile(const std::string& path, int mode) const;

	/**
     * Read entire file content into a string
     * @param path File path to read
     * @return String containing the file's content
     */
	std::string readFile(const std::string& path) const;

	/**
     * Check if a path exists
     * @param path Path to check
     * @return true if the path exists
     */
	bool exists(const std::string& path) const;
};

#endif // FILE_HANDLER_HPP
#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <cstddef>
#include <ostream>
#include <string>
#include <map>
#include <fstream>
#include "Utils.hpp"
#include "LocationConfig.hpp"



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

	// cookies:
	const std::map<std::string, std::string>&	getCookies() const;
	const std::string&	getCookie(const std::string &name) const;

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

	//cookie:
	std::map<std::string, std::string>	_cookies;

};

#endif
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
#include "Cookie.hpp"


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

	// cookies
	void addCookie(const Cookie& c);
	void addSetCookieRaw(const std::string& line); // cgi

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

	// cookies:
	std::vector<Cookie> _setCookies;
	std::vector<std::string> _setCookieLines; // cgi
};

#endif // HTTPRESPONSE_HPP
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
#ifndef LOGGER_HPP
#define LOGGER_HPP

#define COLOR_RESET   "\033[0m"
#define COLOR_DEBUG   "\033[36m"
#define COLOR_INFO    "\033[32m"
#define COLOR_WARNING "\033[33m"
#define COLOR_ERROR   "\033[31m"
#define COLOR_FATAL   "\033[1;31m"
#define COLOR_TIME    "\033[90m"

#define LOG_DEBUG(msg)   Logger::getInstance().debug(msg)
#define LOG_INFO(msg)    Logger::getInstance().info(msg)
#define LOG_WARN(msg) Logger::getInstance().warning(msg)
#define LOG_ERROR(msg)   Logger::getInstance().error(msg)
#define LOG_FATAL(msg)   Logger::getInstance().fatal(msg)

#include <string>
#include <iostream>
#include <ctime>

class Logger 
{
	public:
		enum LogLevel 
		{
			DEBUG,
			INFO,
			WARNING,
			ERROR,
			FATAL
		};

		static Logger& getInstance();

		void setLogLevel(LogLevel level);
		void enableColors(bool enable);

		void debug(const std::string& message);
		void info(const std::string& message);
		void warning(const std::string& message);
		void error(const std::string& message);
		void fatal(const std::string& message);

	private:
		Logger();
		Logger(const Logger&);
		Logger& operator=(const Logger&);

		void log(LogLevel level, const std::string& message);
		std::string getTimestamp();
		std::string getLevelString(LogLevel level);

		LogLevel _currentLevel;
		bool _colorsEnabled;
};


#endif

#ifndef LOGIN_CONTROLLER_HPP
#define LOGIN_CONTROLLER_HPP

#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "SessionManager.hpp"
#include "FileHandler.hpp"
#include <sstream>

static	const	std::string LOGIN_PAGE = "/www/html/login.html";

namespace LoginController
{
	bool handle(HTTPRequest* req, HTTPResponse* res, Session& sess);
}

#endif#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "LocationConfig.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include "../include/EpollManager.hpp"

/**
 * ServerConfig class - Handles server configuration and socket management
 */
#define CLIENT_TIMEOUT 3
#define MAX_CLIENTS 1024
#define MAX_EVENTS 1024
#define BUFFER_SIZE 1024 * 1024

class ServerConfig 
{
private:
	std::string _host;                                // Server host address
	std::vector<uint16_t> _ports;                     // Server ports
	std::string _serverName;                          // Server name
	size_t _clientMaxBodySize;                        // Maximum client body size
	std::string _clientBodyTmpPath;                   // Temporary path for client body
	std::map<int, std::string> _errorPages;           // Custom error pages by status code
	std::map<std::string, LocationConfig> _locations; // Location configurations

	std::vector<int> _server_fds;                     // Server socket file descriptors
	std::vector<struct sockaddr_in> _server_addresses;// Server socket addresses
	EpollManager* _epollManager;

	int createSocket(uint16_t port);

	bool isValidHost(const std::string& host);

public:
	ServerConfig();
	ServerConfig(const ServerConfig& rhs);
	~ServerConfig();
	ServerConfig& operator=(const ServerConfig& rhs);

	void setHost(const std::string& host);
	void setPort(const std::string& portStr);
	void setServerName(const std::string& name);
	void setClientMaxBodySize(const std::string& size);
	void setClientBodyTmpPath(const std::string& path);
	void setErrorPage(const std::string& value);
	void addLocation(const std::string& path, const LocationConfig& location);
	const LocationConfig* findLocation(const std::string& path) const;

	const std::string& getHost() const;
	uint16_t getPort() const;
	const std::vector<uint16_t>& getPorts() const;
	const std::string& getServerName() const;
	size_t getClientMaxBodySize() const;
	std::string getClientBodyTmpPath() const;
	const std::map<int, std::string>& getErrorPages() const;
	std::string getErrorPage(int statusCode) const;
	int getFd(size_t index) const;
	const std::vector<int>& getFds() const;
	const std::map<std::string, LocationConfig>& getLocations() const;

	void setEpollManager(EpollManager* manager) { _epollManager = manager; }
	EpollManager* getEpollManager() const { return _epollManager; }

	void setupServer();
	void cleanupServer();
};

#endif // SERVER_CONFIG_HPP
#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <map>
#include <vector>
#include <string>
#include "EpollManager.hpp"
#include "ServerConfig.hpp"
#include "Client.hpp"
#include "SessionManager.hpp"
#include "LoginController.hpp"

class Client ;

class ServerManager 
{
public:
	// Constructs the ServerManager with a list of server configurations.
	// The timeout, maxClients, and maxEvents default to macros defined in Config.hpp.
	// Note: The caller only provides the servers vector.
	ServerManager(const std::vector<ServerConfig>& servers);

	~ServerManager();

	// Initializes the servers (sets them up and registers their sockets with epoll).
	void startCgi(Client *client);
	void handleCgiResponse(Client *client);
	bool init();

	// Runs the main event loop.
	void run();

	// Stops the event loop.
	void stop();
	void shutdown();

	// Sets the client timeout value (overriding the default macro value).
	void setClientTimeout(int seconds);

	// Returns the count of active client connections.
	size_t getActiveClientCount() const;

private:
	// Looks up the server configuration that owns the given fd.
	ServerConfig* findServerByFd(int fd);
	Client* findClientByFd(int fd);

	// Accepts a new client connection from the given server socket.
	bool acceptClient(int serverFd, ServerConfig* serverConfig);

	// Handles events for a given client.
	void handleClient(int fd, uint32_t events);
	void handleEvent(const epoll_event& event);

	// Reads data from the client.
	bool receiveFromClient(Client* client);

	// Sends response data to the client.
	bool sendToClient(Client* client);

	// Cleans up an individual client.
	void cleanupClient(int fd, const std::string& reason);

	// Periodically checks for and removes timed-out clients.
	void checkClientTimeouts();

	EpollManager _epollManager;
	bool         _running;
	int          _clientTimeout;
	int          _maxClients;
	int _epollFd;

	// Stored server configurations (as provided in the constructor).
	std::vector<ServerConfig> _servers;

	// A map of server socket file descriptor to its ServerConfig.
	std::map<int, ServerConfig*> _serverMap;

	// Active client connections mapped by their file descriptor.
	std::map<int, Client*> _clients;

	std::string _buffer;
};

#endif // SERVERMANAGER_HPP
#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include <ctime>


struct Session 
{
    std::map<std::string, std::string> data;
    time_t created;
    time_t lastAccess;
};

class SessionManager
{
    public:
        static SessionManager& get();

        Session& acquire(HTTPRequest* req, HTTPResponse* res);
        void reapExpired(const time_t& now);
        void printSession();

    private:
        SessionManager();
        std::string generateSessionID();
        bool isValidSessionID(const std::string& sid);

        std::map<std::string, Session> _sessions;
        size_t _maxAge;         // total lifetime in seconds (e.g. 12h)
        size_t _idleTimeout;    // idle timeout in seconds (e.g. 30min)
};

#endif/* ************************************************************************** */
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

	// cookie:
	std::map<std::string,std::string> parseCookieHeader(const std::string& h);

}


#endif

#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include "EpollManager.hpp"
#include "ServerConfig.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include "ServerConfig.hpp"
#include "EpollManager.hpp"
#include "Logger.hpp"
#include "ServerManager.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"


#define QUEUE_CONNEXION 5
#define TIMEOUT_CHECK_INTERVAL 10
#define CONNECTION_TIMEOUT 60 

  static const size_t MAX_URI_LENGTH = 8192;  // 8KB is common server limit
      static const size_t MAX_QUERY_LENGTH = 4096; // Optional separate limit for queriesh
  static const size_t MAX_BODY_SIZE = 100000000; // Optional separate limit for queriesh






#endif
