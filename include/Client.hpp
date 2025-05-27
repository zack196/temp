#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>
#include <netinet/in.h>
#include <sys/types.h>
#include "ServerConfig.hpp"
#include "HTTPRequest.hpp"
#include <cstring>
#include "HTTPResponse.hpp"


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
};

#endif // CLIENT_HPP
