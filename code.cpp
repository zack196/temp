#include "../include/CGIHandler.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/HTTPResponse.hpp"
#include "../include/Client.hpp"
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <fcntl.h>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

CGIHandler::CGIHandler(HTTPRequest* request, HTTPResponse* response)
	: _pid(-1)
	, _request(request)
	, _response(response)
	, _envp(NULL)
	, _argv(NULL)
	, _startTime(time(NULL))
	, _timeout(30)
	, _pipeClosed(false)
{
	if (!request || !response)
		throw std::runtime_error("Invalid request or response");

	_execPath = _request->getLocation()->getCgiPath();
	_scriptPath = _request->getResource();

	validatePaths();

	if (pipe(_pipeFd) == -1)
		throw std::runtime_error("Failed to create pipe: " + std::string(strerror(errno)));
}

CGIHandler::~CGIHandler()
{
	cleanup();
}

void CGIHandler::validatePaths() const
{
	struct stat st;

	if (_execPath.empty() || _scriptPath.empty())
		throw std::runtime_error("Empty CGI paths");

	if (stat(_execPath.c_str(), &st) == -1)
		throw std::runtime_error("Cannot access interpreter: " + _execPath);

	if (!(st.st_mode & S_IXUSR))
		throw std::runtime_error("Interpreter not executable: " + _execPath);

	if (stat(_scriptPath.c_str(), &st) == -1)
		throw std::runtime_error("Cannot access script: " + _scriptPath);

	if (!(st.st_mode & S_IRUSR))
		throw std::runtime_error("Script not readable: " + _scriptPath);
}

void CGIHandler::setEnv()
{
	cleanEnv();
	_env.clear();

	_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	_env["SERVER_SOFTWARE"] = "Webserv/1.0";
	_env["REQUEST_METHOD"] = _request->getMethod();
	_env["SCRIPT_NAME"] = _scriptPath;
	_env["SCRIPT_FILENAME"] = _scriptPath;
	_env["PATH_INFO"] = _request->getPath();
	_env["PATH_TRANSLATED"] = _scriptPath;
	_env["QUERY_STRING"] = _request->getQuery();
	_env["SERVER_PROTOCOL"] = _request->getProtocol();
	_env["HTTP_HOST"] = _request->getHeader("host");
	_env["CONTENT_LENGTH"] = Utils::toString(_request->getContentLength());
	_env["CONTENT_TYPE"] = _request->getHeader("content-type");

	const std::map<std::string, std::string>& headers = _request->getHeaders();
	std::map<std::string, std::string>::const_iterator it;
	for (it = headers.begin(); it != headers.end(); ++it)
	{
		if (it->first == "host" || it->first == "content-type")
			continue;

		std::string envName = "HTTP_";
		for (std::string::size_type i = 0; i < it->first.size(); ++i)
		{
			char c = it->first[i];
			envName += (c == '-') ? '_' : static_cast<char>(std::toupper(c));
		}
		_env[envName] = it->second;
	}

	_env["PYTHONIOENCODING"] = "utf-8";
}

void CGIHandler::buildEnv()
{
	cleanEnv();
	_envp = new char*[_env.size() + 1];

	std::map<std::string, std::string>::const_iterator it;
	int index = 0;

	for (it = _env.begin(); it != _env.end(); ++it)
	{
		std::string envVar = it->first + "=" + it->second;
		_envp[index++] = strdup(envVar.c_str());
	}
	_envp[index] = NULL;
}

void CGIHandler::buildArgv()
{
	cleanArgv();
	_argv = new char*[3];
		_argv[0] = strdup(_execPath.c_str());
	_argv[1] = strdup(_scriptPath.c_str());
	_argv[2] = NULL;
}

void CGIHandler::cleanEnv()
{
	if (_envp)
	{
		for (int i = 0; _envp[i] != NULL; ++i)
			free(_envp[i]);
		delete[] _envp;
		_envp = NULL;
	}
}

void CGIHandler::cleanArgv()
{
	if (_argv)
	{
		for (int i = 0; _argv[i] != NULL; ++i)
			free(_argv[i]);
		delete[] _argv;
		_argv = NULL;
	}
}

void CGIHandler::start()
{
	buildEnv();
	buildArgv();

	_pid = fork();
	if (_pid < 0)
		throw std::runtime_error("Fork failed: " + std::string(strerror(errno)));

	if (_pid == 0) // Child process
	{
		// Open input file for body if it exists
		int inFd = open(_request->getBodyfile().c_str(), O_RDONLY);
		if (inFd == -1)
		{
			std::cerr << "Failed to open input file: " << strerror(errno) << std::endl;
			_exit(EXIT_FAILURE);
		}

		// Setup pipes
		close(_pipeFd[0]);  // Close read end
		if (dup2(inFd, STDIN_FILENO) == -1 || dup2(_pipeFd[1], STDOUT_FILENO) == -1)
		{
			std::cerr << "Failed to setup pipes: " << strerror(errno) << std::endl;
			_exit(EXIT_FAILURE);
		}

		close(inFd);
		close(_pipeFd[1]);

		execve(_execPath.c_str(), _argv, _envp);
		std::cerr << "execve failed: " << strerror(errno) << std::endl;
		_exit(EXIT_FAILURE);
	}

	// Parent process
	close(_pipeFd[1]);  // Close write end

	// Set non-blocking read
	int flags = fcntl(_pipeFd[0], F_GETFL, 0);
	if (flags != -1)
		fcntl(_pipeFd[0], F_SETFL, flags | O_NONBLOCK);
}

void CGIHandler::cleanup()
{
	if (_pid > 0)
	{
		kill(_pid, SIGTERM);
		_pid = -1;
	}

	if (!_pipeClosed && _pipeFd[0] >= 0)
	{
		close(_pipeFd[0]);
		_pipeFd[0] = -1;
		_pipeClosed = true;
	}

	cleanEnv();
	cleanArgv();
}

int CGIHandler::getPipeFd() const
{
	return _pipeFd[0];
}

pid_t CGIHandler::getPid() const
{
	return _pid;
}
#include "../include/Client.hpp"
#include "../include/Logger.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/CGIHandler.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>


Client::Client(int fd, ServerConfig* server)
	: _fd(fd),
	_server(server),
	_request(this->getServer()),
	_response(this->getRequest()),
	_cgiHandler(NULL),
	_isCgi(false),
	_bytesSent(0),
	_headersSize(0),
	_lastActivity(time(NULL)),
	_cgiStartTime(0),
	_readBuffer("")
{
	if (fd < 0 || !server)
		throw std::runtime_error("Invalid client parameters");
}

Client::~Client()
{
	if (_cgiHandler)
	{
		_isCgi = false;
		_cgiHandler->cleanup(); // ensure the child process is terminated
		delete _cgiHandler;
		_cgiHandler = NULL;
	}

	if (_fileStream.is_open())
		_fileStream.close();

	if (_fd >= 0)
		close(_fd);

	LOG_DEBUG("Client destroyed, fd: " + Utils::toString(_fd));
}

std::string& Client::getReadBuffer()
{
	return _readBuffer;
}

void Client::setReadBuffer(const char* data, ssize_t bytesSet)
{
	_readBuffer.append(data, bytesSet);
}

ssize_t Client::getResponseChunk(char* buffer, size_t bufferSize)
{
	if (!buffer)
		return -1;

	const size_t totalResponseSize = _response.getHeader().size() + _response.getContentLength();

	if (_bytesSent >= totalResponseSize)
		return 0;

	else if (_bytesSent < _response.getHeader().size())
		return getHeaderResponse(buffer);

	else if (!_response.getBody().empty())
		return getStringBodyResponse(buffer);

	else if (!_response.getFilePath().empty())
		return getFileBodyResponse(buffer, bufferSize);

	return -1;
}

ssize_t Client::getHeaderResponse(char* buffer)
{
	const std::string& header = _response.getHeader();
	std::memcpy(buffer, header.c_str(), header.size());
	_bytesSent += header.size();
	_headersSize = header.size();
	return header.size();
}

ssize_t Client::getStringBodyResponse(char* buffer)
{
	const std::string& body = _response.getBody();
	std::memcpy(buffer, body.c_str(), body.size());
	_bytesSent += body.size();
	return body.size();
}

ssize_t Client::getFileBodyResponse(char* buffer, size_t bufferSize)
{
	const std::string& filePath = _response.getFilePath();

	if (!_fileStream.is_open())
	{
		_fileStream.open(filePath.c_str(), std::ios::binary);
		if (!_fileStream.is_open())
			return -1;
	}
	_fileStream.read(buffer, bufferSize);
	ssize_t bytesRead = _fileStream.gcount();

	if (bytesRead > 0)
	{
		_bytesSent += bytesRead;
		return bytesRead;
	}

	_fileStream.close();
	return 0;
}

int Client::getFd() const
{
	return _fd;
}

time_t Client::getLastActivity() const
{
	return _lastActivity;
}

void Client::updateActivity()
{
	_lastActivity = time(NULL);
}

HTTPRequest* Client::getRequest()
{
	return &_request;
}

HTTPResponse* Client::getResponse()
{
	return &_response;
}

ServerConfig* Client::getServer() const
{
	return _server;
}

bool Client::shouldKeepAlive() const
{
	return !_response.shouldCloseConnection();
}

bool Client::hasTimedOut(time_t currentTime, time_t timeout) const
{
	if (_isCgi) 
		return (currentTime - _cgiStartTime) > timeout;
	else 
		return (currentTime - _lastActivity) > timeout;
}

void Client::reset()
{
	_request.clear();
	_response.clear();
	_readBuffer.clear();
	_bytesSent = 0;
	_headersSize = 0;

	if (_fileStream.is_open())
		_fileStream.close();

	if (_cgiHandler)
	{
		_cgiHandler->cleanup();
		delete _cgiHandler;
		_cgiHandler = NULL;
	}
	_isCgi = false;

	updateActivity();
	// LOG_DEBUG("Client reset for keep-alive, fd: " + Utils::toString(_fd));
}

void Client::setCgiHandler(CGIHandler* cgiHandler)
{
	_cgiHandler = cgiHandler;
}

CGIHandler* Client::getCgiHandler() const
{
	return _cgiHandler;
}

void Client::setIsCgi(bool isCgi)
{
	_isCgi = isCgi;
}

bool Client::isCgi() const
{
	return _isCgi;
}

void Client::setSession(Session* s)
{
	_session = s;
}

Session*    Client::getSession() const
{
	return _session;
}#include "../include/ConfigParser.hpp"
#include "../include/Utils.hpp"
#include <sstream>
#include <cctype>

ConfigParser::ConfigParser() : _configFile(), _servers() 
{
}

ConfigParser::ConfigParser(const std::string& configFilePath) : _configFile(configFilePath), _servers() 
{
}

ConfigParser::ConfigParser(const ConfigParser& rhs) : _configFile(rhs._configFile), _servers(rhs._servers)
{
}

ConfigParser& ConfigParser::operator=(const ConfigParser& rhs) 
{
	if (this != &rhs) 
	{
		_configFile = rhs._configFile;
		_servers = rhs._servers;
	}
	return *this;
}

void ConfigParser::parseFile() 
{
	const std::string& path = _configFile.getPath();
	validateFilePath(path);

	std::string content = _configFile.readFile(path);
	cleanContent(content);
	parseServerBlocks(content);
}

void ConfigParser::removeComments(std::string& content) 
{
	size_t pos = 0;
	while ((pos = content.find('#', pos)) != std::string::npos) 
	{
		size_t end = content.find('\n', pos);
		content.erase(pos, end != std::string::npos ? end - pos : std::string::npos);
	}
}

void ConfigParser::trimWhitespace(std::string& content) 
{
	// Remove leading whitespace
	size_t start = content.find_first_not_of(" \t\r\n");
	if (start != std::string::npos) 
		content.erase(0, start);
	else 
	{
		content.clear();
		return;
	}

	// Remove trailing whitespace
	size_t end = content.find_last_not_of(" \t\r\n");
	if (end != std::string::npos) 
		content.erase(end + 1);
}

void ConfigParser::collapseSpaces(std::string& content) 
{
	std::string result;
	result.reserve(content.size());  // Optimize for performance

	bool inSpace = false;
	bool inNewline = false;

	for (size_t i = 0; i < content.size(); ++i) 
	{
		if (content[i] == '\n') 
		{
			if (!inNewline) 
			{
				result += '\n';
				inNewline = true;
				inSpace = false;
			}
		}
		else if (std::isspace(content[i])) 
		{
			if (!inSpace && !inNewline) 
			{
				result += ' ';
				inSpace = true;
			}
		}
		else 
		{
			result += content[i];
			inSpace = false;
			inNewline = false;
		}
	}

	size_t pos = result.find_last_not_of(" \t\r\n");
	if (pos != std::string::npos) 
		result.erase(pos + 1);

	else 
		result.clear();

	content = result;
}

void ConfigParser::cleanContent(std::string& content) 
{
	removeComments(content);
	trimWhitespace(content);
	collapseSpaces(content);
}

void ConfigParser::validateFilePath(const std::string& path) 
{
	int fileType = _configFile.getTypePath(path);

	switch (fileType) 
	{
		case 0:
			throw std::runtime_error("Configuration file does not exist: " + path);
		case 2:
			throw std::runtime_error("Path is a directory, not a file: " + path);
		case 3:
			throw std::runtime_error("Path is a special file: " + path);
		case 1:
			break; // Regular file, no action needed
		default:
			throw std::runtime_error("Unknown file type: " + path);
	}

	if (!_configFile.checkFile(path, 0)) 
		throw std::runtime_error("No read permission for file: " + path);
}

void ConfigParser::parseServerBlocks(const std::string& content) 
{
	_servers.clear();

	size_t pos = 0;
	while ((pos = content.find("server", pos)) != std::string::npos) 
	{
		size_t blockStart = findBlockStart(content, pos);
		size_t blockEnd = findBlockEnd(content, blockStart);

		std::string block = content.substr(blockStart + 1, blockEnd - blockStart - 2);
		parseServerBlock(block);
		pos = blockEnd;
	}
}

size_t ConfigParser::findBlockStart(const std::string& content, size_t pos) 
{
	size_t blockStart = content.find("{", pos);
	if (blockStart == std::string::npos) 
		throw std::runtime_error("Missing '{' in server block");

	return blockStart;
}

size_t ConfigParser::findBlockEnd(const std::string& content, size_t blockStart) 
{
	int braceCount = 1;
	size_t blockEnd = blockStart + 1;

	while (braceCount > 0 && blockEnd < content.size()) 
	{
		if (content[blockEnd] == '{') 
			braceCount++;

		else if (content[blockEnd] == '}') 
			braceCount--;

		blockEnd++;
	}

	if (braceCount != 0) 
		throw std::runtime_error("Unmatched '{' in server block");

	return blockEnd;
}

void ConfigParser::parseServerBlock(const std::string& block) 
{
	ServerConfig server;
	LocationConfig defaultLocation;

	server.addLocation("/", defaultLocation);
	size_t pos = 0;

	while (pos < block.size()) 
	{
		while (pos < block.size() && std::isspace(block[pos])) 
			++pos;

		if (pos >= block.size()) 
			break;

		// Find directive end
		size_t endDirective = block.find_first_of(";\n{", pos);
		if (endDirective == std::string::npos) 
		{
			break;
		}

		std::string directiveLine = Utils::trim(block.substr(pos, endDirective - pos));
		if (directiveLine.empty()) 
		{
			pos = endDirective + 1;
			continue;
		}

		size_t spacePos = directiveLine.find_first_of(" \t");
		if (spacePos == std::string::npos) 
		{
			pos = endDirective + 1;
			continue;
		}

		std::string key = Utils::trim(directiveLine.substr(0, spacePos));
		std::string value = Utils::trim(directiveLine.substr(spacePos + 1));

		if (key == "location") 
		{
			size_t openBrace = block.find('{', endDirective);
			if (openBrace == std::string::npos) 
				throw std::runtime_error("Missing '{' in location block");

			int braceCount = 1;
			size_t closeBrace = openBrace + 1;
			while (braceCount > 0 && closeBrace < block.size()) 
			{
				if (block[closeBrace] == '{') 
					braceCount++;

				else if (block[closeBrace] == '}') 
					braceCount--;

				closeBrace++;
			}

			if (braceCount != 0) 
				throw std::runtime_error("Unmatched '{' in location block");

			std::string locationBlock = block.substr(openBrace + 1, closeBrace - openBrace - 2);
			LocationConfig location = parseLocationBlock(value, locationBlock);
			server.addLocation(value, location);
			pos = closeBrace;
		} 
		else 
		{
			if (key == "host") 
				server.setHost(value);
			else if (key == "listen") 
				server.setPort(value);
			else if (key == "server_name") 
				server.setServerName(value);
			else if (key == "client_max_body_size") 
				server.setClientMaxBodySize(value);
			else if (key == "client_body_temp_path") 
				server.setClientBodyTmpPath(value);
			else if (key == "error_page") 
				server.setErrorPage(value);

			pos = endDirective + 1;
		}
	}

	_servers.push_back(server);  // Add to vector
}

LocationConfig ConfigParser::parseLocationBlock(const std::string& locationHeader, 
						const std::string& block) 
{
	LocationConfig location;
	std::istringstream iss(block);
	std::string line;

	location.setPath(locationHeader);

	while (std::getline(iss, line)) 
	{
		line = Utils::trim(line);
		if (line.empty() || line == "}") 
			continue;

		size_t pos = line.find_first_of(" \t");
		if (pos == std::string::npos) 
			throw std::runtime_error("Invalid config line in location block: " + line);

		std::string key = Utils::trim(line.substr(0, pos));
		std::string value = Utils::trim(line.substr(pos + 1));

		// Remove trailing semicolon or opening brace
		if (!value.empty() && (value[value.size() - 1] == ';' || value[value.size() - 1] == '{')) 
			value = value.substr(0, value.size() - 1);

		if (key == "root") 
			location.setRoot(value);
		else if (key == "return") 
			location.setRedirect(value);
		else if (key == "index") 
			location.setIndex(value);
		else if (key == "autoindex") 
			location.setAutoindex(value);
		else if (key == "allow_methods") 
			location.setAllowedMethods(value);
		else if (key == "cgi_extension") 
			location.setCgiExtension(value);
		else if (key == "upload_path") 
			location.setUploadPath(value);
		else if (key == "cgi_path") 
			location.setCgiPath(value);
	}

	return location;
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter) 
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream iss(str);

	while (std::getline(iss, token, delimiter)) 
	{
		token = Utils::trim(token);
		if (!token.empty()) 
			tokens.push_back(token);
	}
	return tokens;
}
#include "../include/Cookie.hpp"

Cookie::Cookie(const std::string& name, const std::string& value) 
    :_name(name), _value(value), _expires(0), _secure(false), _httpOnly(false) {}


static std::string formatExpires(time_t t) 
{
    // Format: Wdy, DD Mon YYYY HH:MM:SS GMT
    char buf[100];
    struct tm *gmt = gmtime(&t);
    if (gmt) {
        strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
        return std::string(buf);
    }
    return "";
}

std::string Cookie::toSetCookieString() const 
{
    std::ostringstream  oss;
    oss << _name << "=" << _value;
    if (!_path.empty()) 
        oss << "; Path=" << _path;
    if (!_domain.empty())
        oss << "; Domain=" << _domain;
    if (_expires != 0)
        oss << "; Expires=" << formatExpires(_expires);
    if (_secure)
        oss << "; Secure";
    if (_httpOnly)
        oss << "; HttpOnly";
    if (!_sameSite.empty())
        oss << "; SameSite=" << _sameSite;
    return oss.str();
}
#include "../include/EpollManager.hpp"
#include <stdexcept>
#include <cstring>
#include <errno.h>
#include <unistd.h>

EpollManager::EpollManager(size_t maxEvents) : _epollFd(-1) , _events(maxEvents)
{
	_epollFd = epoll_create(1);
	if (_epollFd == -1)
		throw std::runtime_error(std::string("Failed to create epoll: ") + strerror(errno));
}

EpollManager::~EpollManager()
{
	if (_epollFd != -1)
	{
		close(_epollFd);
		_epollFd = -1;
	}
}

bool EpollManager::add(int fd, uint32_t events)
{
	if (_epollFd == -1 || fd < 0)
		return false;

	epoll_event ev = {};
	ev.events = events;
	ev.data.fd = fd;

	return (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) != -1);
}

bool EpollManager::modify(int fd, uint32_t events)
{
	if (_epollFd == -1 || fd < 0)
		return false;

	epoll_event ev = {};
	ev.events = events;
	ev.data.fd = fd;

	return (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) != -1);
}

bool EpollManager::remove(int fd)
{
	if (_epollFd == -1 || fd < 0)
		return false;

	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
		if (errno != EBADF && errno != ENOENT)
			return false;

	return true;
}

int EpollManager::getFd() const
{
	return _epollFd;
}


int EpollManager::wait(int timeout)
{
	if (_epollFd == -1)
		return -1;

	int numEvents = epoll_wait(_epollFd, _events.data(), _events.size(), timeout);

	if (numEvents == -1 && errno != EINTR)
		throw std::runtime_error(std::string("epoll_wait failed: ") + 
			   strerror(errno));

	return numEvents;
}

const epoll_event& EpollManager::getEvent(size_t index) const
{
	if (index >= _events.size())
		throw std::out_of_range("EpollManager: Event index out of range");
	return _events[index];
}
#include "../include/FileHandler.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>

/**
 * Default constructor
 */
FileHandler::FileHandler() : _filePath("") {
}

/**
 * Constructor with file path
 */
FileHandler::FileHandler(const std::string& filePath) : _filePath(filePath) {
}

/**
 * Copy constructor
 */
FileHandler::FileHandler(const FileHandler& other) : _filePath(other._filePath) {
}

/**
 * Assignment operator
 */
FileHandler& FileHandler::operator=(const FileHandler& other) {
	if (this != &other) {
		_filePath = other._filePath;
	}
	return *this;
}

/**
 * Destructor
 */
FileHandler::~FileHandler() {
}

/**
 * Get the file path
 */
std::string FileHandler::getPath() const {
	return _filePath;
}

/**
 * Set the file path
 */
void FileHandler::setPath(const std::string& filePath) {
	_filePath = filePath;
}

/**
 * Get the type of the path:
 * 0 - does not exist
 * 1 - regular file
 * 2 - directory
 * 3 - special file (device, pipe, socket, etc.)
 */
int FileHandler::getTypePath(const std::string& path) const {
	struct stat fileStat;

	// Check if the file exists
	if (stat(path.c_str(), &fileStat) != 0) {
		return 0; // Does not exist
	}

	// Check the file type
	if (S_ISREG(fileStat.st_mode)) {
		return 1; // Regular file
	} else if (S_ISDIR(fileStat.st_mode)) {
		return 2; // Directory
	} else {
		return 3; // Special file (device, pipe, socket, etc.)
	}
}

/**
 * Check if a file has specific permissions
 * @param path File path to check
 * @param mode 0 for readable, 1 for writable, 2 for executable
 * @return true if the file has the requested permission
 */
bool FileHandler::checkFile(const std::string& path, int mode) const {
	int accessMode;

	switch (mode) {
		case 0:
			accessMode = R_OK; // Check for read permission
			break;
		case 1:
			accessMode = W_OK; // Check for write permission
			break;
		case 2:
			accessMode = X_OK; // Check for execute permission
			break;
		default:
			accessMode = F_OK; // Check for existence only
			break;
	}

	return (access(path.c_str(), accessMode) == 0);
}

/**
 * Read entire file content into a string
 * @param path File path to read
 * @return String containing the file's content
 */
std::string FileHandler::readFile(const std::string& path) const 
{
	std::ifstream file(path.c_str());

	if (!file.is_open()) 
		throw std::runtime_error("Failed to open file: " + path);

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

/**
 * Check if a path exists
 * @param path Path to check
 * @return true if the path exists
 */
bool FileHandler::exists(const std::string& path) const {
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}
#include "../include/HTTPRequest.hpp"
#include "../include/Logger.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/LocationConfig.hpp"
#include "../include/Utils.hpp"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <cstring>
#include <string>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <sstream>

HTTPRequest::HTTPRequest(ServerConfig* server)
	: _server(server),
	_statusCode(0),
	_state(INIT),
	_parsePosition(0),
	_contentLength(0),
	_method(""),
	_uri("/"),
	_path(""),
	_query(""),
	_protocol("HTTP/1.1"),
	_bodyBuffer(""),
	_headerKey(""),
	_headerValue(""),
	_headers(),
	_contentType(""),
	_boundary(""),
	_resource(""),
	_isChunked(false),
	_keepAlive(true),
	_chunkSize(-1),
	_receivedLength(0),
	_multipartBoundary(""),
	_location(NULL),
	_uploadHeadersParsed(false),
	_chunkFileInitialized(false),
	_uploadFilepath(""),
	_uploadBoundary(""),
	_multipartState(PART_HEADER),
	_chunkState(CHUNK_SIZE),
	_hasCgi(false),
	_bodyTempFile("")
{
}

HTTPRequest::HTTPRequest(const HTTPRequest& other)
	: _server(other._server),
	_statusCode(other._statusCode),
	_state(other._state),
	_parsePosition(other._parsePosition),
	_contentLength(other._contentLength),
	_method(other._method),
	_uri(other._uri),
	_path(other._path),
	_query(other._query),
	_protocol(other._protocol),
	_bodyBuffer(other._bodyBuffer),
	_headerKey(other._headerKey),
	_headerValue(other._headerValue),
	_headers(other._headers),
	_contentType(other._contentType),
	_boundary(other._boundary),
	_resource(other._resource),
	_isChunked(other._isChunked),
	_keepAlive(other._keepAlive),
	_chunkSize(other._chunkSize),
	_receivedLength(other._receivedLength),
	_multipartBoundary(other._multipartBoundary),
	_location(other._location),
	_uploadHeadersParsed(other._uploadHeadersParsed),
	_chunkFileInitialized(other._chunkFileInitialized),
	_uploadFilepath(other._uploadFilepath),
	_uploadBoundary(other._uploadBoundary),
	_multipartState(other._multipartState),
	_chunkState(other._chunkState),
	_hasCgi(other._hasCgi),
	_bodyTempFile(other._bodyTempFile)
{
}

HTTPRequest& HTTPRequest::operator=(const HTTPRequest& other) 
{
	if (this != &other) 
	{
		closeOpenFiles();
		_server               = other._server;
		_statusCode           = other._statusCode;
		_state                = other._state;
		_parsePosition        = other._parsePosition;
		_contentLength        = other._contentLength;
		_method               = other._method;
		_uri                  = other._uri;
		_path                 = other._path;
		_query                = other._query;
		_protocol             = other._protocol;
		_bodyBuffer           = other._bodyBuffer;
		_headerKey            = other._headerKey;
		_headerValue          = other._headerValue;
		_headers              = other._headers;
		_contentType          = other._contentType;
		_boundary             = other._boundary;
		_resource             = other._resource;
		_isChunked            = other._isChunked;
		_keepAlive            = other._keepAlive;
		_chunkSize            = other._chunkSize;
		_receivedLength       = other._receivedLength;
		_multipartBoundary    = other._multipartBoundary;
		_location             = other._location;
		_uploadHeadersParsed  = other._uploadHeadersParsed;
		_chunkFileInitialized = other._chunkFileInitialized;
		_uploadFilepath       = other._uploadFilepath;
		_uploadBoundary       = other._uploadBoundary;
		_multipartState       = other._multipartState;
		_chunkState           = other._chunkState;
		_hasCgi               = other._hasCgi;
		_bodyTempFile        = other._bodyTempFile;
	}
	return *this;
}

HTTPRequest::~HTTPRequest() 
{
	closeOpenFiles();
}

void HTTPRequest::closeOpenFiles() 
{
	if (_uploadFile.is_open())
		_uploadFile.close();
	if (_bodyStream.is_open())
		_bodyStream.close();
}

bool HTTPRequest::keepAlive() const 
{
	return _keepAlive;
}

const std::map<std::string, std::string>& HTTPRequest::getHeaders() const 
{
	return _headers;
}

bool HTTPRequest::isComplete() const 
{
	return (_state == FINISH || _state == ERRORE);
}


bool HTTPRequest::hasCgi()
{
	std::string path = _resource;

	size_t dotPos = path.find_last_of('.');
	if (dotPos == std::string::npos)
		return false;

	std::string extension = path.substr(dotPos);

	if (_location && _location->getCgiExtension() == extension)
		return true;

	return false;
}

const std::string& HTTPRequest::getUri() const 
{
	return _uri;
}

const std::string& HTTPRequest::getProtocol() const 
{
	return _protocol;
}

const std::string& HTTPRequest::getQuery() const 
{
	return _query;
}

const std::string& HTTPRequest::getBody() const 
{
	return _bodyBuffer;
}

const std::string& HTTPRequest::getMethod() const 
{
	return _method;
}

const std::string& HTTPRequest::getResource() const 
{
	return _resource;
}

const std::string& HTTPRequest::getPath() const 
{
	return _path;
}

int HTTPRequest::getState() const 
{
	return _state;
}

int HTTPRequest::getStatusCode() const 
{
	return _statusCode;
}

ServerConfig* HTTPRequest::getServer() const 
{
    return _server;
}

size_t HTTPRequest::getContentLength() const 
{
	return _contentLength;
}

const std::string& HTTPRequest::getHeader(const std::string& key) const 
{
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	static std::string empty;
	return (it != _headers.end()) ? it->second : empty;
}

void HTTPRequest::setStatusCode(int code) 
{
	_statusCode = code;
}

void HTTPRequest::setState(ParseState state) 
{
	_state = state;
}

std::string HTTPRequest::getQueryParameter(const std::string& key) const 
{
	std::map<std::string, std::string>::const_iterator it = _queryParameters.find(key);
	if (it != _queryParameters.end()) 
		return it->second;
	return "";
}

const LocationConfig* HTTPRequest::getLocation() const 
{
	return _location;
}

void HTTPRequest::parse(std::string& rawdata) 
{
	if (_state == FINISH || _state == ERRORE || rawdata.empty())
		return;
	if (_state == INIT)
		_state = LINE_METHOD;
	if (_state == LINE_METHOD)
		parseMethod(rawdata);
	if (_state == LINE_URI)
		parseUri(rawdata);
	if (_state == LINE_VERSION)
		parseVersion(rawdata);
	if (_state == HEADER_KEY)
		parseHeadersKey(rawdata);
	if (_state == HEADER_VALUE)
		parseHeadersValue(rawdata);
	if (_state == BODY_INIT || _state == BODY_MULTIPART || _state == BODY_CGI)
		parseRequestBody(rawdata);

}

void HTTPRequest::parseMethod(std::string& rawdata) 
{
	size_t space_pos = rawdata.find(' ');
	if (space_pos == std::string::npos)
		return;
	_method = rawdata.substr(0, space_pos);
	if (_method.empty() || !Utils::isValidMethodToken(_method)) 
	{
		setState(ERRORE);
		setStatusCode(400);
		return;
	}
	if (!Utils::isSupportedMethod(_method)) 
	{
		setState(ERRORE);
		setStatusCode(400);
		return;
	}
	rawdata.erase(0, space_pos + 1);
	setState(LINE_URI);
}

void HTTPRequest::parseUri(std::string& rawdata) 
{
	size_t space_pos = rawdata.find(' ');
	if (space_pos == std::string::npos)
		return;
	_uri = rawdata.substr(0, space_pos);
	if (_uri.size() > 8192) 
	{
		setState(ERRORE);
		setStatusCode(414);
		return;
	}
	if (_uri.empty() || !Utils::isValidUri(_uri)) 
	{
		setState(ERRORE);
		setStatusCode(400);
		return;
	}
	if (Utils::urlDecode(_uri) == -1) 
	{
		setState(ERRORE);
		setStatusCode(400);
		return;
	}
	size_t query_pos = _uri.find('?');
	if (query_pos != std::string::npos) 
	{
		_path = _uri.substr(0, query_pos);
		_query = _uri.substr(query_pos + 1);
		parseQueryParameters(_query);
	} 
	else 
	{
		_path = _uri;
		_query.clear();
	}
	rawdata.erase(0, space_pos + 1);

	_location = _server->findLocation(_path);
	_resource = _location->getResource(_path);
	setState(LINE_VERSION);
}

void HTTPRequest::parseQueryParameters(const std::string& query)
{
	size_t start = 0;
	size_t end;
	while ((end = query.find('&', start)) != std::string::npos) 
	{
		std::string param = query.substr(start, end - start);
		size_t equals = param.find('=');
		if (equals != std::string::npos) 
		{
			std::string key = param.substr(0, equals);
			std::string value = param.substr(equals + 1);
			_queryParameters[key] = value;
			if(key == "method")
				_method = value;
		}
		start = end + 1;
	}

	std::string param = query.substr(start);
	size_t equals = param.find('=');
	if (equals != std::string::npos) 
	{
		std::string key = param.substr(0, equals);
		std::string value = param.substr(equals + 1);
		_queryParameters[key] = value;
		if(key == "method")
			_method = value;
	}
}

void HTTPRequest::parseVersion(std::string& rawdata) 
{
	size_t pos = rawdata.find("\r\n");
	if (pos == std::string::npos)
		pos = rawdata.find("\n");
	if (pos == std::string::npos)
		return;
	_protocol = rawdata.substr(0, pos);
	if (!Utils::isValidVersion(_protocol) || _protocol != "HTTP/1.1") 
	{
		setState(ERRORE);
		setStatusCode(400);
		return;
	}
	rawdata.erase(0, pos + ((rawdata[pos]=='\r') ? 2 : 1));
	setState(HEADER_KEY);
}

void HTTPRequest::parseHeadersKey(std::string& rawdata) 
{
	if (rawdata.empty())
		return;
	bool isCRLF = (rawdata.size() >= 2 && rawdata.substr(0,2) == "\r\n");
	bool isLF = (!isCRLF && rawdata.size() >= 1 && rawdata[0] == '\n');
	if (isCRLF || isLF) 
	{
		if (_headers.empty() || _headers.find("host") == _headers.end()) 
		{
			setState(ERRORE);
			setStatusCode(400);
			return;
		}
		rawdata.erase(0, (isCRLF ? 2 : 1));
		setState(BODY_INIT);
		return;
	}
	size_t colon_pos = rawdata.find(':');
	if (colon_pos == std::string::npos)
		return;
	_headerKey = rawdata.substr(0, colon_pos);
	if (!Utils::isValidHeaderKey(_headerKey)) 
	{
		setState(ERRORE);
		setStatusCode(400);
		return;
	}
	rawdata.erase(0, colon_pos + 1);
	setState(HEADER_VALUE);
}

void HTTPRequest::parseHeadersValue(std::string& rawdata) 
{
	size_t start = Utils::skipLeadingWhitespace(rawdata);
	rawdata.erase(0, start);
	size_t pos = rawdata.find("\r\n");
	if (pos == std::string::npos)
		pos = rawdata.find("\n");
	if (pos == std::string::npos)
		return;
	_headerValue = rawdata.substr(0, pos);
	if (!Utils::isValidHeaderValue(_headerValue)) 
	{
		setState(ERRORE);
		setStatusCode(400);
		return;
	}
	for (size_t i = 0; i < _headerKey.size(); ++i)
		_headerKey[i] = std::tolower(_headerKey[i]);
	if (_headers.find(_headerKey) != _headers.end()) 
	{
		if (_headerKey == "host" || _headerKey == "content-length" || _headerKey == "transfer-encoding" || _headerKey == "connection" || _headerKey == "expect") 
		{
			setState(ERRORE);
			setStatusCode(400);
			return;
		}
		else if (_headerKey == "cookie" || _headerKey == "accept" || _headerKey == "accept-language") 
			_headers[_headerKey] += "; " + _headerValue;
		else
			_headers[_headerKey] = _headerValue;
	} 
	else 
		_headers[_headerKey] = _headerValue;

	// cookie:
	if (_headerKey == "cookie")
	{
		_cookies = Utils::parseCookieHeader(_headerValue);
	}

	_headerKey.erase();
	_headerValue.erase();
	rawdata.erase(0, pos + ((rawdata[pos]=='\r') ? 2 : 1));
	setState(HEADER_KEY);
	parse(rawdata);
}


const std::string& HTTPRequest::getBodyfile() const
{
	return _bodyTempFile;
}

bool HTTPRequest::validateCgi()
{
	if (!hasCgi())
		return true;
	_bodyTempFile = Utils::createTempFile("body_", _server->getClientBodyTmpPath());

	_bodyStream.open(_bodyTempFile.c_str(), std::ios::binary);
	if (!_bodyStream.is_open())
	{
		LOG_ERROR("Failed to open CGI body file: " + _bodyTempFile);
		setStatusCode(500);
		setState(ERRORE);
		return false;
	}

	LOG_INFO("Created CGI body temp file: " + _bodyTempFile);
	_state = BODY_CGI;
	return true;
}

void HTTPRequest::writeBodyToFile(std::string& rawdata) 
{
	if (!_bodyStream.is_open()) 
	{
		LOG_ERROR("CGI body file is not open. Unable to write.");
		setStatusCode(500);
		setState(ERRORE);
		return;
	}

	_bodyStream.write(rawdata.c_str(), rawdata.size());
	if (!_bodyStream.good()) 
	{
		LOG_ERROR("Failed to write to CGI body file: " + _bodyTempFile);
		setStatusCode(500);
		setState(ERRORE);
		_bodyStream.close();
		return;
	}

	_receivedLength += rawdata.size();

	rawdata.clear();

	if (_receivedLength >= _contentLength) 
	{
		LOG_INFO("Completed writing CGI body to file: " + _bodyTempFile);
		_bodyStream.close();
		setStatusCode(201);
		setState(FINISH);
	}
}

void HTTPRequest::parseRequestBody(std::string& rawdata) 
{

	if (_state == BODY_INIT) 
	{
		if (!validateAllowedMethods() ||
			!validateHostHeader() ||
			!validateContentLength() ||
			!validateTransferEncoding() ||
			!validateMultipartFormData() ||
			!validateCgi())
			return;
	}

	/* ----------  NEW: ordinary body -------------------- */
    if (_state == BODY_INIT && !_isChunked && _multipartBoundary.empty())
    {
        // append as much as we still need
        size_t need = _contentLength - _receivedLength;   // may be 0
        size_t take = std::min(need, rawdata.size());

        _bodyBuffer.append(rawdata, 0, take);
        _receivedLength += take;
        rawdata.erase(0, take);

        if (_receivedLength < _contentLength)
            return;                       // wait for more TCP segments

        /* body complete */
        setStatusCode(200);
        setState(FINISH);
        return;
    }


	if (_state == BODY_CGI)
		writeBodyToFile(rawdata);

	else if (_state == BODY_MULTIPART)
		parseMultipartBody(rawdata);

	else if (_state == BODY_CHUNKED)
		parseChunkBody(rawdata);
	else 
	{
		setStatusCode(200);
		setState(FINISH);
	}
}


void HTTPRequest::parseChunkBody(std::string& rawdata) 
{
	if (!_chunkFileInitialized) 
	{
		_bodyTempFile = Utils::createTempFile("chunke_", _server->getClientBodyTmpPath());
		_bodyStream.open(_bodyTempFile.c_str(), std::ios::binary);
		if (!_bodyStream.is_open()) 
		{
			setStatusCode(500);
			setState(ERRORE);
			return;
		}
		_chunkFileInitialized = true;
		_chunkState = CHUNK_SIZE;
	}
	while (!rawdata.empty()) 
	{
		if (_chunkState == CHUNK_SIZE) 
		{
			if (!processChunkSize(rawdata))
				return;
		}
		else if (_chunkState == CHUNK_DATA) 
		{
			if (!processChunkData(rawdata))
				return;
		}
		else if (_chunkState == CHUNK_FINISHED)
			return;
	}
}

bool HTTPRequest::processChunkSize(std::string& rawdata) 
{
	size_t pos = rawdata.find("\r\n");
	if (pos == std::string::npos)
		return false;
	if (pos == 0 || pos > 8) 
	{
		setStatusCode(400);
		setState(ERRORE);
		_bodyStream.close();
		return false;
	}
	_chunkSize = 0;
	for (size_t i = 0; i < pos; ++i) 
	{
		char c = rawdata[i];
		if (c >= '0' && c <= '9')
			_chunkSize = (_chunkSize * 16) + (c - '0');
		else if (c >= 'a' && c <= 'f')
			_chunkSize = (_chunkSize * 16) + (c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			_chunkSize = (_chunkSize * 16) + (c - 'A' + 10);
		else 
		{
			setStatusCode(400);
			setState(ERRORE);
			_bodyStream.close();
			return false;
		}
	}
	rawdata.erase(0, pos + 2);
	if (_chunkSize == 0) 
	{
		_bodyStream.close();
		_chunkState = CHUNK_FINISHED;
		setStatusCode(201);
		setState(FINISH);
		return false;
	}
	_chunkState = CHUNK_DATA;
	return true;
}

bool HTTPRequest::processChunkData(std::string& rawdata) 
{
	if (rawdata.size() < static_cast<size_t>(_chunkSize + 2))
		return false;

	if (rawdata[_chunkSize] != '\r' || rawdata[_chunkSize + 1] != '\n') 
	{
		setStatusCode(400);
		setState(ERRORE);
		_bodyStream.close();
		return false;
	}
	_bodyStream.write(rawdata.c_str(), _chunkSize);
	if (!_bodyStream.good()) 
	{
		setStatusCode(500);
		setState(ERRORE);
		_bodyStream.close();
		return false;
	}
	rawdata.erase(0, _chunkSize + 2);
	_chunkState = CHUNK_SIZE;
	return true;
}

void HTTPRequest::parseMultipartBody(std::string& rawdata) 
{
	while (!rawdata.empty()) 
	{
		if (_multipartState == PART_HEADER) 
		{
			if (!processPartHeader(rawdata))
				return;
		}
		else if (_multipartState == PART_DATA) 
		{
			if (!processPartData(rawdata))
				return;
		}
		else if (_multipartState == PART_BOUNDARY) 
		{
			if (!processPartBoundary(rawdata))
				return;
		} 
		else if (_multipartState == PART_END) 
		{
			setStatusCode(201);
			return;
		}
	}
}

bool HTTPRequest::processPartHeader(std::string& rawdata) 
{
	size_t end = rawdata.find("\r\n\r\n");
	if (end == std::string::npos) 
	{
		setStatusCode(400);
		setState(ERRORE);
		return false;
	}

	std::string headers = rawdata.substr(0, end);
	rawdata.erase(0, end + 4);
	size_t filename_pos = headers.find("filename=\"");
	if (filename_pos == std::string::npos) 
	{
		setStatusCode(400);
		setState(ERRORE);
		return false;
	}
	filename_pos += 10;
	size_t quote_end = headers.find("\"", filename_pos);
	if (quote_end == std::string::npos) 
	{
		setStatusCode(400);
		setState(ERRORE);
		return false;
	}
	std::string filename = headers.substr(filename_pos, quote_end - filename_pos);
	for (size_t i = 0; i < filename.size(); ++i)
		if (!std::isalnum(filename[i]) && filename[i] != '.' && filename[i] != '_')
			filename[i] = '_';
	_uploadFilepath = _location->getUploadPath() + "/" + filename;
	_uploadFile.open(_uploadFilepath.c_str(), std::ios::binary);
	if (!_uploadFile.is_open()) 
	{
		setStatusCode(500);
		setState(ERRORE);
		return false;
	}
	_multipartState = PART_DATA;
	return true;
}

bool HTTPRequest::processPartData(std::string& rawdata) 
{
	size_t boundary_pos = rawdata.find(_uploadBoundary);
	if (boundary_pos == std::string::npos) 
	{
		size_t reserve = _uploadBoundary.size() + 4;
		if (rawdata.size() <= reserve)
			return false;
		size_t write_len = rawdata.size() - reserve;
		_uploadFile.write(rawdata.c_str(), write_len);
		_uploadFile.flush();
		rawdata.erase(0, write_len);
		return false;
	}
	size_t data_end = boundary_pos;
	if (boundary_pos >= 2 && rawdata.substr(boundary_pos - 2, 2) == "\r\n")
		data_end -= 2;
	if (data_end > 0) 
	{
		_uploadFile.write(rawdata.c_str(), data_end);
		_uploadFile.flush();
	}
	_uploadFile.close();
	rawdata.erase(0, boundary_pos);
	_multipartState = PART_BOUNDARY;
	return true;
}

bool HTTPRequest::processPartBoundary(std::string& rawdata) 
{
	if (rawdata.substr(0, _uploadBoundary.length()) != _uploadBoundary) 
	{
		setStatusCode(400);
		setState(ERRORE);
		return false;
	}
	rawdata.erase(0, _uploadBoundary.length());
	if (rawdata.substr(0, 2) == "--") 
	{
		rawdata.erase(0, 2);
		_multipartState = PART_END;
		setState(FINISH);
		setStatusCode(201);
		return true;
	}
	else if (rawdata.substr(0, 2) == "\r\n") 
	{
		rawdata.erase(0, 2);
		_uploadFilepath.erase();
		_multipartState = PART_HEADER;
		return true;
	}
	setStatusCode(400);
	setState(ERRORE);
	return false;
}

bool HTTPRequest::validateHostHeader() 
{
	if (getHeader("host").empty()) 
	{
		setStatusCode(400);
		setState(ERRORE);
		return false;
	}
	return true;
}

bool HTTPRequest::validateContentLength() 
{
	std::string cl = getHeader("content-length");
	if (cl.empty())
		return true;
	for (size_t i = 0; i < cl.size(); ++i)
		if (!std::isdigit(cl[i])) 
		{
			setStatusCode(400);
			setState(ERRORE);
			return false;
		}
	_contentLength = Utils::stringToSizeT(cl);
	if (_contentLength > _server->getClientMaxBodySize()) 
	{
		setStatusCode(413);
		setState(ERRORE);
		return false;
	}
	return true;
}

bool HTTPRequest::validateTransferEncoding() 
{
	std::string te = getHeader("transfer-encoding");
	std::string cl = getHeader("content-length");
	if (!te.empty()) 
	{
		if (te != "chunked") 
		{
			setStatusCode(501);
			setState(ERRORE);
			return false;
		}
		if (!cl.empty()) 
		{
			setStatusCode(400);
			setState(ERRORE);
			return false;
		}
		_isChunked = true;
		_state = BODY_CHUNKED;
		_chunkState = CHUNK_SIZE;
	}
	return true;
}

bool HTTPRequest::validateMultipartFormData() 
{
	if (_location && _location->hasCgi())
		return true;
	std::string ct = getHeader("content-type");
	if (ct.find("multipart/form-data") == std::string::npos)
		return true;
	size_t pos = ct.find("boundary=");
	if (pos == std::string::npos) 
	{
		setStatusCode(469);
		setState(ERRORE);
		return false;
	}
	std::string boundary = ct.substr(pos + 9);
	if (boundary.empty()) 
	{
		setStatusCode(469);
		setState(ERRORE);
		return false;
	}
	_uploadBoundary = "--" + boundary;
	_multipartState = PART_HEADER;
	_state = BODY_MULTIPART;
	return true;
}

bool HTTPRequest::validateAllowedMethods() 
{
	if (_location && _location->isMethodAllowed(_method))
		return true;
	setState(ERRORE);
	setStatusCode(405);
	return false;
}

void HTTPRequest::clear() 
{
	if (_uploadFile.is_open())
		_uploadFile.close();
	if (_bodyStream.is_open())
		_bodyStream.close();

	if (std::remove(_bodyTempFile.c_str()) == 0) 
		LOG_INFO("Removed temporary body file: " + _bodyTempFile);

	_statusCode = 0;
	_state = INIT;
	_parsePosition = 0;
	_contentLength = 0;
	_method = "GET";
	_uri = "/";
	_path = "";
	_query = "";
	_protocol = "HTTP/1.1";
	_bodyBuffer = "";
	_headerKey = "";
	_headerValue = "";
	_headers.clear();
	_contentType = "";
	_boundary = "";
	_isChunked = false;
	_keepAlive = true;
	_chunkSize = -1;
	_multipartBoundary = "";
	_location = NULL;
	_uploadHeadersParsed = false;
	_chunkFileInitialized = false;
	_uploadFilepath = "";
	_uploadBoundary = "";
	_multipartState = PART_HEADER;
	_chunkState = CHUNK_SIZE;
	_hasCgi = false;

}

const std::map<std::string, std::string>&	HTTPRequest::getCookies() const
{
	return _cookies;
}

const std::string&	HTTPRequest::getCookie(const std::string &name) const
{
	static const std::string	empty;
	std::map<std::string, std::string>::const_iterator it = _cookies.find(name);
	return ( (it == _cookies.end()) ? empty : it->second );
}#include "../include/HTTPResponse.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/HTTPRequest.hpp"
#include "../include/Utils.hpp"
#include "../include/Logger.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <errno.h>
#include <ostream>
#include <strings.h>
#include <sys/wait.h>

HTTPResponse::HTTPResponse(HTTPRequest* request)
	: _request(request),
	_protocol("HTTP/1.1"),
	_statusCode(200),
	_statusMessage("OK"),
	_body(""),
	_header(""),
	_filePath(""),
	_fileSize(0),
	_state(INIT),
	_cgiHeaderComplete(false),
	_hasCgiOutput(false)
{
	if (!_request)
		throw std::runtime_error("Invalid HTTPRequest pointer in HTTPResponse constructor");
}

HTTPResponse::HTTPResponse(const HTTPResponse& rhs) 
{
	*this = rhs;
}

HTTPResponse& HTTPResponse::operator=(const HTTPResponse& rhs) 
{
	if (this != &rhs) 
	{
		_request       = rhs._request;
		_protocol      = rhs._protocol;
		_statusCode    = rhs._statusCode;
		_statusMessage = rhs._statusMessage;
		_headers       = rhs._headers;
		_body          = rhs._body;
		_header        = rhs._header;
		_filePath      = rhs._filePath;
		_fileSize      = rhs._fileSize;
		_state         = rhs._state;
	}
	return *this;
}

HTTPResponse::~HTTPResponse()
{
	if (_cgiStream.is_open())
		_cgiStream.close();
}

void HTTPResponse::clear() 
{
	_protocol      = "HTTP/1.1";
	_statusCode    = 200;
	_statusMessage = "OK";
	_headers.clear();
	_body.clear();
	_header.clear();
	_filePath.clear();
	_fileSize      = 0;
	_state         = INIT;
	if (_cgiStream.is_open())
		_cgiStream.close();

	if (!_cgiTempFile.empty() && std::remove(_cgiTempFile.c_str()) == 0) 
		LOG_INFO("Removed temporary body file: " + _cgiTempFile);

	_hasCgiOutput = false;
	_cgiHeaderComplete = false;

}

void HTTPResponse::setProtocol(const std::string& protocol) 
{
	_protocol = protocol;
}

void HTTPResponse::setStatusCode(int code) 
{
	_statusCode = code;
}

void HTTPResponse::setStatusMessage(const std::string& message) 
{
	_statusMessage = message;
}

void HTTPResponse::setHeader(const std::string& key, const std::string& value) 
{
	_headers[key] = value;
}

void HTTPResponse::appendToBody(const std::string& bodyPart) 
{
	_body.append(bodyPart);
}

void HTTPResponse::setBodyResponse(const std::string& filePath) 
{
	_filePath = filePath;
	struct stat file_stat;
	if (stat(filePath.c_str(), &file_stat) == -1) 
	{
		LOG_ERROR("Failed to stat file " + filePath + ": " + std::string(strerror(errno)));
		throw std::runtime_error("Failed to stat file: " + filePath);
	}
	_fileSize = file_stat.st_size;
}

void HTTPResponse::setState(response_state state) 
{
	_state = state;
}

std::string HTTPResponse::getHeader() const 
{
	return _header;
}

std::string HTTPResponse::getBody() const 
{
	return _body;
}

std::string HTTPResponse::getFilePath() const 
{
	return _filePath;
}

size_t HTTPResponse::getContentLength() const 
{
	return !_body.empty() ? _body.size() : _fileSize;
}

size_t HTTPResponse::getFileSize() const 
{
	return _fileSize;
}

int HTTPResponse::getState() const 
{
	return _state;
}

bool HTTPResponse::shouldCloseConnection() const 
{
	int reqStatus = _request->getStatusCode();
	std::string connection = Utils::trim(_request->getHeader("connection"));

	if (connection == "close")
		return true;
	else if (connection == "keep-alive")
		return false;
	else if (reqStatus == 400 || reqStatus == 408 || reqStatus == 414 || reqStatus == 431 || reqStatus == 501 || reqStatus == 505)
		return true;
	return false;
}

void HTTPResponse::buildHeader()
{
	std::ostringstream oss;
	oss << _protocol << " " << _statusCode << " " << _statusMessage << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
		oss << it->first << ": " << it->second << "\r\n";
	
	// cookies:
	for (std::vector<Cookie>::iterator it = _setCookies.begin(); it != _setCookies.end(); it++ )
		oss << "Set-Cookie: " << it->toSetCookieString() << "\r\n";
	
	for (std::vector<std::string>::iterator it = _setCookieLines.begin(); it != _setCookieLines.end(); it++ )
		oss << "Set-Cookie: " << *it << "\r\n";
	
	oss << "\r\n";
	_header = oss.str();
}


void HTTPResponse::buildResponse() 
{
	if (_request->getState() == HTTPRequest::ERRORE)
		buildErrorResponse(_request->getStatusCode());
	else if (_request->hasCgi())
		handleCgi();
	else if (_request->getLocation() && _request->getLocation()->hasRedirection())
		handleRedirect();
	else if (_request->getMethod() == "GET")
		handleGet();
	else if (_request->getMethod() == "POST")
		handlePost();
	else if (_request->getMethod() == "DELETE")
		handleDelete();
	else
		buildErrorResponse(405);
}

void HTTPResponse::buildErrorResponse(int statusCode) 
{
	std::string defaultPage = _request->getServer()->getErrorPage(statusCode);
	setProtocol(_request->getProtocol());
	setStatusCode(statusCode);
	setStatusMessage(Utils::getMessage(statusCode));
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Content-Type", "text/html");

	if (Utils::fileExists(defaultPage)) 
	{
		setBodyResponse(defaultPage);
		setHeader("Content-Length", Utils::toString(getFileSize()));
	}
	else 
	{
		appendToBody(defaultPage);
		setHeader("Content-Length", Utils::toString(_body.size()));
	}
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");
	buildHeader();
	setState(HEADER_SENT);
}

void HTTPResponse::buildSuccessResponse(const std::string& fullPath) 
{
	setProtocol(_request->getProtocol());
	setStatusCode(_request->getStatusCode());
	setStatusMessage(Utils::getMessage(_request->getStatusCode()));
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");
	setHeader("Content-Type", Utils::getMimeType(fullPath));
	setBodyResponse(fullPath);
	setHeader("Content-Length", Utils::toString(getContentLength()));
	buildHeader();
	setState(FINISH);
}

void HTTPResponse::handleGet() 
{
	std::string resource = _request->getResource();
	std::cout << "resource = " << resource << std::endl;
	if (resource.empty()) 
		return(buildErrorResponse(404));

	else if (_request->getLocation() && _request->getLocation()->isAutoIndexOn() && Utils::isDirectory(resource)) 
		return(handleAutoIndex());

	else if (Utils::fileExists(resource)) 
		return(buildSuccessResponse(resource));
	else 
		return(buildErrorResponse(403));
}


void  HTTPResponse::handleCgi()
{
	setStatusCode(200);
	setStatusMessage("Created");
	setHeader("Content-Type", "text/html");
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");
	setHeader("Content-Length", Utils::toString(getContentLength()));
	buildHeader();
	setState(FINISH);
	
}

void HTTPResponse::handlePost() 
{
	std::string resource = _request->getResource();
	if (resource.empty() || !Utils::fileExists(resource)) 
		return(buildErrorResponse(404));

	std::string body = "Resource created successfully\n";
	setProtocol(_request->getProtocol());
	setStatusCode(201);
	setStatusMessage("Created");
	setHeader("Content-Type", "text/plain");
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");
	appendToBody(body);
	setHeader("Content-Length", Utils::toString(_body.size()));
	buildHeader();
	setState(FINISH);

}

void HTTPResponse::handleDelete()
{
	std::string resource = _request->getResource();
	if (resource.empty() || !Utils::fileExists(resource)) 
		return(buildErrorResponse(404));

	if (Utils::isDirectory(resource)) 
		return (buildErrorResponse(403));

	if (std::remove(resource.c_str()) != 0) 
	{
		LOG_ERROR("Failed to delete file " + resource + ": " + std::string(strerror(errno)));
		return (buildErrorResponse(500));
	}
	setProtocol(_request->getProtocol());
	setStatusCode(200);
	setStatusMessage("OK");
	setHeader("Content-Type", "text/plain");
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");
	appendToBody("Resource deleted successfully\n");
	setHeader("Content-Length", Utils::toString(_body.size()));
	buildHeader();
	setState(FINISH);
}

void HTTPResponse::handleRedirect() 
{
	const LocationConfig* location = _request->getLocation();
	if (!location || !location->hasRedirection()) 
	{
		return (buildErrorResponse(500));
	}
	int code = location->getRedirectCode();
	std::string reason = Utils::getMessage(code);
	std::string target = location->getRedirectPath();
	setProtocol(_request->getProtocol());
	setStatusCode(code);
	setStatusMessage(reason);
	setHeader("Content-Type", "text/html");
	setHeader("Location", target);
	setHeader("Server", "1337webserver");
	setHeader("Date", Utils::getCurrentDate());
	setHeader("Connection", shouldCloseConnection() ? "close" : "keep-alive");

	appendToBody("<html><head><title>");
	appendToBody(Utils::toString(code));
	appendToBody(" ");
	appendToBody(reason);
	appendToBody("</title></head><body><h1>");
	appendToBody(Utils::toString(code));
	appendToBody(" ");
	appendToBody(reason);
	appendToBody("</h1><hr><center>");
	appendToBody("1337webserver");
	appendToBody("</center></body></html>");

	setHeader("Content-Length", Utils::toString(_body.size()));
	buildHeader();
	setState(HEADER_SENT);
}

void HTTPResponse::handleAutoIndex() 
{
	std::string directory_path = _request->getResource();
	if (!Utils::isDirectory(directory_path)) 
		return(buildErrorResponse(404));

	std::vector<std::string> entries = Utils::listDirectory(directory_path);
	std::string req_path = _request->getPath();
	if (req_path.empty() || req_path[req_path.size()-1] != '/')
		req_path += '/';
	// Build an HTML directory listing.
	appendToBody("<!DOCTYPE html><html><head>");
	appendToBody("<title>Index of ");
	appendToBody(req_path);
	appendToBody("</title></head><body><h1>Index of ");
	appendToBody(req_path);
	appendToBody("</h1><hr><pre>");
	if (req_path != "/")
		appendToBody("<a href=\"../\">../</a>\n");
	for (size_t i = 0; i < entries.size(); ++i) 
	{
		std::string full_path = directory_path + "/" + entries[i];
		bool is_dir = Utils::isDirectory(full_path);
		std::string link_name = entries[i] + (is_dir ? "/" : "");
		appendToBody("<a href=\"" + req_path + link_name + "\">" + link_name + "</a>\n");
	}
	appendToBody("</pre><hr></body></html>");
	setProtocol(_request->getProtocol());
	setStatusCode(200);
	setStatusMessage("OK");
	setHeader("Content-Type", "text/html");
	setHeader("Content-Length", Utils::toString(_body.size()));
	buildHeader();
}

void HTTPResponse::parseCgiOutput(const std::string& chunk)
{
	static std::string headerBuffer;

	if (_cgiHeaderComplete) 
		return (writeToFile(chunk));

	headerBuffer += chunk;

	size_t headerEnd = headerBuffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		headerEnd = headerBuffer.find("\n\n");

	if (headerEnd != std::string::npos) 
	{
		std::string headers = headerBuffer.substr(0, headerEnd);
		bool hasValidHeaders = parseHeaders(headers);

		size_t bodyStart = headerEnd + (headerBuffer.find("\r\n\r\n") != std::string::npos ? 4 : 2);
		std::string body = headerBuffer.substr(bodyStart);
		if (!body.empty())
			writeToFile(body);

		_cgiHeaderComplete = hasValidHeaders;
		headerBuffer.clear();
	} 
	else if (headerBuffer.size() >= 8192) 
		buildErrorResponse(502);
	else 
	{
		writeToFile(headerBuffer);
		headerBuffer.clear();
		_cgiHeaderComplete = true;
	}
	setBodyResponse(_cgiTempFile);
}

bool HTTPResponse::parseHeaders(const std::string& headers)
{
	std::istringstream headerStream(headers);
	std::string line;
	bool hasValidHeaders = false;

	while (std::getline(headerStream, line)) 
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		if (line.empty())
			continue;

		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos) 
		{
			hasValidHeaders = true;
			std::string key = line.substr(0, colonPos);
			std::string value = line.substr(colonPos + 1);

			value.erase(0, value.find_first_not_of(" \t"));
			value.erase(value.find_last_not_of(" \t") + 1);

			// handel cgi Set-Cookie
			if (key == "Set-Cookie")
			{
				addSetCookieRaw(value);
				continue ;
			}

			setHeader(key, value);
			LOG_DEBUG("Parsed CGI Header: " + key + ": " + value);
		}
	}

	return hasValidHeaders;
}

void HTTPResponse::writeToFile(const std::string& data)
{
	if (data.empty())
		return;

	if (!_hasCgiOutput)
		createCgiTempFile();

	_cgiStream.write(data.c_str(), data.size());
	_cgiStream.flush();

	if (!_cgiStream)
		throw std::runtime_error("Failed to write data to CGI temp file: " + _cgiTempFile);
}

void HTTPResponse::createCgiTempFile()
{
	if (_hasCgiOutput)
		return;

	_cgiTempFile = Utils::createTempFile("cgi_response_", _request->getServer()->getClientBodyTmpPath());

	if (_cgiStream.is_open())
		_cgiStream.close();

	_cgiStream.open(_cgiTempFile.c_str(), std::ios::out | std::ios::binary);
	if (!_cgiStream.is_open())
		throw std::runtime_error("Failed to open CGI temp file: " + _cgiTempFile);

	_hasCgiOutput = true;
	LOG_INFO("Created CGI temp file: " + _cgiTempFile);
}

void HTTPResponse::closeCgiTempFile()
{
	if (_cgiStream.is_open())
	{
		_cgiStream.flush(); // Ensure all data is written
		_cgiStream.close(); // Close the file
		LOG_INFO("Closed CGI temp file: " + _cgiTempFile);
	}
}

void HTTPResponse::addCookie(const Cookie& c)
{
	_setCookies.push_back(c);
}

void HTTPResponse::addSetCookieRaw(const std::string& line)
{
	_setCookieLines.push_back(line);
}#include "../include/LocationConfig.hpp"
#include "../include/Utils.hpp"
#include <sstream>
#include <iostream>

LocationConfig::LocationConfig() : _root("./www/html"), _path("/"), _index("index.html"), _autoindex(false), _cgiExtension(""), _cgiPath(""), _uploadPath("")
{
	_redirect = std::make_pair(0, "");
}

LocationConfig::~LocationConfig()
{
}

LocationConfig::LocationConfig(const LocationConfig& other) 
{
	*this = other;
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other)
{
	if (this != &other)
	{
		_root = other._root;
		_path = other._path;
		_index = other._index;
		_autoindex = other._autoindex;
		_allowedMethods = other._allowedMethods;
		_cgiExtension = other._cgiExtension;
		_cgiPath = other._cgiPath;
		_redirect = other._redirect;
		_uploadPath = other._uploadPath;
	}
	return *this;
}

void LocationConfig::setRoot(const std::string& root)
{
	_root = root;
}

void LocationConfig::setPath(const std::string& path)
{
	_path = path;
}

void LocationConfig::setIndex(const std::string& index)
{
	_index = index;
}

void LocationConfig::setAutoindex(const std::string& autoindex) 
{
	if (autoindex == "on") 
		_autoindex = true;
	else if (autoindex == "off") 
		_autoindex = false;
	else 
		throw std::runtime_error("Invalid autoindex value: " + autoindex + " (must be 'on' or 'off')");
}

void LocationConfig::setUploadPath(const std::string& path)
{
	_uploadPath = path;
}

std::vector<std::string> LocationConfig::split(const std::string& str, char delimiter) 
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream iss(str);

	while (std::getline(iss, token, delimiter)) 
	{
		token = Utils::trim(token);
		if (!token.empty()) 
			tokens.push_back(token);
	}

	return tokens;
}

std::string LocationConfig::getResource(const std::string& requestPath) const 
{
	std::string root = _root;
	std::string path = requestPath;

	if (!root.empty() && root[root.length() - 1] != '/') 
		root += '/';

	if (!path.empty() && path[0] == '/') 
		path = path.substr(1);

	std::string full_path = root + path;

	if (Utils::isDirectory(full_path)) 
	{
		std::string index_path = full_path;
		if (!index_path.empty() && index_path[index_path.length() - 1] != '/') 
			index_path += '/';

		index_path += _index;
		if (Utils::fileExists(index_path)) 
			return index_path;

		return full_path;
	}

	if (Utils::fileExists(full_path)) 
		return full_path;

	return "";
}

void LocationConfig::setAllowedMethods(const std::string& methods)
{
	std::vector<std::string> methodsList = split(methods, ' ');
	_allowedMethods.clear();

	for (std::vector<std::string>::iterator it = methodsList.begin(); it != methodsList.end(); ++it)
	{
		std::string method = *it;

		if (method == "GET" || method == "POST" || method == "DELETE") 
			_allowedMethods.push_back(method);
		else 
			throw std::runtime_error("Invalid HTTP method: " + method + " (allowed: GET, POST, DELETE)");
	}
}

void LocationConfig::setCgiExtension(const std::string& extension)
{ 
	_cgiExtension = extension;
}

void LocationConfig::setCgiPath(const std::string& path)
{
	_cgiPath = path;
}

void LocationConfig::setRedirect(const std::string& redirectValue) 
{
	std::istringstream iss(redirectValue);
	int code;
	std::string url;

	if (!(iss >> code >> url)) 
		throw std::runtime_error("Invalid redirect format: " + redirectValue + 
			   " (should be 'code url')");

	if (code < 300 || code > 308) 
		throw std::runtime_error("Invalid redirect code: " + Utils::toString(code) + " (must be between 300-308)");

	if (!url.empty() && url[url.length()-1] == ';') 
		url.erase(url.length()-1);

	_redirect = std::make_pair(code, url);
}

const std::string& LocationConfig::getRoot() const 
{
	return _root;
}

const std::string& LocationConfig::getPath() const 
{
	return _path;
}

const std::string& LocationConfig::getIndex() const
{
	return _index;
}

bool LocationConfig::getAutoindex() const
{
	return _autoindex;
}

bool LocationConfig::isAutoIndexOn() const 
{
	return _autoindex;
}

const std::vector<std::string>& LocationConfig::getAllowedMethods() const
{
	return _allowedMethods;
}

const std::pair<int, std::string>& LocationConfig::getRedirect() const 
{
	return _redirect;
}

int LocationConfig::getRedirectCode() const 
{
	return _redirect.first;
}

const std::string& LocationConfig::getRedirectPath() const 
{
	return _redirect.second;
}

const std::string& LocationConfig::getCgiExtension() const
{
	return _cgiExtension; 
}

const std::string& LocationConfig::getCgiPath() const
{
	return _cgiPath;
}

const std::string& LocationConfig::getUploadPath() const
{
	return _uploadPath;
}

bool LocationConfig::isMethodAllowed(const std::string& method) const
{
	if (_allowedMethods.empty())
		return true;

	for (std::vector<std::string>::const_iterator it = _allowedMethods.begin(); 
		it != _allowedMethods.end(); ++it) 
		if (*it == method)
			return true;

	return false;
}

bool LocationConfig::hasRedirection() const 
{
	return (_redirect.first >= 300 && _redirect.first <= 308 && !_redirect.second.empty());
}

bool LocationConfig::hasCgi() const 
{
	return (!_cgiExtension.empty() && !_cgiPath.empty());
}

bool LocationConfig::allowsUploads() const
{
	return !_uploadPath.empty();
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/09 21:51:45 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/10 14:34:11 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Logger.hpp"

Logger& Logger::getInstance() 
{
    static Logger instance;
    return instance;
}

Logger::Logger() : _currentLevel(INFO), _colorsEnabled(true)
{
}

void Logger::setLogLevel(LogLevel level) 
{
    _currentLevel = level;
}

void Logger::enableColors(bool enable) 
{
    _colorsEnabled = enable;
}

void Logger::debug(const std::string& message) 
{
    log(DEBUG, message);
}

void Logger::info(const std::string& message)
{
    log(INFO, message);
}

void Logger::warning(const std::string& message)
{
    log(WARNING, message);
}

void Logger::error(const std::string& message)
{
    log(ERROR, message);
}

void Logger::fatal(const std::string& message)
{
    log(FATAL, message);
}

void Logger::log(LogLevel level, const std::string& message)
{

    std::cerr << (_colorsEnabled ? COLOR_TIME : "") << "[" << getTimestamp() << "] " << (_colorsEnabled ? COLOR_RESET : "");
    
    switch(level)
    {
        case DEBUG:
            std::cerr << (_colorsEnabled ? COLOR_DEBUG : "") << "[DEBUG] ";
            break;
        case INFO:
            std::cerr << (_colorsEnabled ? COLOR_INFO : "") << "[INFO] ";
            break;
        case WARNING:
            std::cerr << (_colorsEnabled ? COLOR_WARNING : "") << "[WARNING] ";
            break;
        case ERROR:
            std::cerr << (_colorsEnabled ? COLOR_ERROR : "") << "[ERROR] ";
            break;
        case FATAL:
            std::cerr << (_colorsEnabled ? COLOR_FATAL : "") << "[FATAL] ";
            break;
    }
    
    std::cerr << message << (_colorsEnabled ? COLOR_RESET : "") << std::endl;
}

std::string Logger::getTimestamp() 
{
    time_t now;
    time(&now);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

std::string Logger::getLevelString(LogLevel level)
{
    switch(level)
    {
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        case FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}
#include "../include/LoginController.hpp"

static bool validUser(const std::string& u, const std::string& p)
{
    // TODO ta tbadalha b chi data base wala chi 7aja
    return (u == "alice" && p == "123") ||
           (u == "bob"   && p == "qwerty");
}

bool	LoginController::handle(HTTPRequest* req, HTTPResponse* res, Session& sess)
{
	const std::string path = req->getPath();
	const std::string methode = req->getMethod();

	if (path == "/login" && methode == "GET" && !sess.data.count("user"))
		return false;
	if (path == "/login" && methode == "POST")
	{
		std::map<std::string, std::string> kv;
		std::string body = req->getBody();
		std::cout << "body = " << body << std::endl;
		Utils::urlDecode(body);
		
		std::stringstream ss(body);
		std::string tok;
		while (getline(ss, tok, '&'))
		{
			size_t eq = tok.find('=');
			if (eq == std::string::npos)
				continue ;
			kv[tok.substr(0, eq)] = tok.substr(eq + 1);
		}
		std::string user = kv["username"];
		std::string pass = kv["password"];
		
		std::cout << "user = " << user << std::endl;
		std::cout << "pass = " << pass << std::endl;

		if (validUser(user, pass))
		{
			sess.data["user"] = user;
			res->setStatusCode(302);
			res->setStatusMessage("Found");
			res->setHeader("Location", "/");
		}
		else
		{
			res->setStatusCode(200);
    		res->setStatusMessage("OK");
    		res->setHeader("Content-Type","text/html");
    		res->setBodyResponse("<script>document.getElementById('err').textContent=" 
				"'Invalid credentials';</script>");
		}
		return true;
	}

	if (path == "/logout")
	{
		sess.data.clear();
		res->setStatusCode(302);
		res->setStatusMessage("Found");
		res->setHeader("Location","/login");
		return true;
	}

	if (path.rfind("/secret",0) == 0 && !sess.data.count("user"))
    {
        res->setStatusCode(302);
        res->setStatusMessage("Found");
        res->setHeader("Location","/login");
        return true;
    }

	return false;
}
#include "../include/ConfigParser.hpp"
#include "../include/ServerManager.hpp"
#include "../include/Logger.hpp"
#include <signal.h>

ServerManager* globalServer = NULL;

void signal_handler(int signum) 
{
	if (globalServer) 
	{
		LOG_INFO("Signal " + Utils::toString(signum) + " received. Stopping server gracefully...");
		globalServer->stop();
	}
}


int main(int argc, char* argv[])
{
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	try 
	{
		std::string configFile = "./config/default.conf";
		if (argc > 1)
			configFile = argv[1];

		ConfigParser parser(configFile);
		parser.parseFile();

		std::vector<ServerConfig> servers = parser.getServers();
		if (servers.empty()) 
		{
			LOG_ERROR("No valid server configurations found");
			return 1;
		}

		ServerManager serverManager(servers);
		globalServer = &serverManager;

		if (!serverManager.init()) 
		{
			LOG_ERROR("Failed to initialize server");
			return 1;
		}

		serverManager.run();
		return 0;
		
	} 
	catch (const std::exception& e) 
	{
		LOG_ERROR("Fatal error: " + std::string(e.what()));
		return 1;
	}
}
#include "../include/ServerConfig.hpp"
#include "../include/Utils.hpp"
#include <netinet/in.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdexcept>

ServerConfig::ServerConfig() : _host("0.0.0.0"), _serverName(""), _clientMaxBodySize(1024 * 1024) //1MB
{
}

ServerConfig::~ServerConfig() 
{
	cleanupServer();
}

ServerConfig::ServerConfig(const ServerConfig& rhs) : _host(""), _serverName(""), _clientMaxBodySize(0)
{
	*this = rhs;
}

ServerConfig& ServerConfig::operator=(const ServerConfig& rhs) 
{
	if (this != &rhs) 
	{
		cleanupServer();

		_host = rhs._host;
		_ports = rhs._ports;
		_serverName = rhs._serverName;
		_clientMaxBodySize = rhs._clientMaxBodySize;
		_clientBodyTmpPath = rhs._clientBodyTmpPath;
		_errorPages = rhs._errorPages;
		_locations = rhs._locations;
	}
	return *this;
}

const LocationConfig* ServerConfig::findLocation(const std::string& path) const 
{
	const std::map<std::string, LocationConfig>& locations = getLocations();
	const LocationConfig* bestMatch = NULL;
	size_t bestLength = 0;
	for (std::map<std::string, LocationConfig>::const_iterator it = locations.begin();
	it != locations.end(); ++it)
	{
		const std::string& locPath = it->first;
		if (path.compare(0, locPath.length(), locPath) == 0 &&
			locPath.length() > bestLength &&
			(path.length() == locPath.length() || path[locPath.length()] == '/' || locPath == "/"))
		{
			bestMatch = &it->second;
			bestLength = locPath.length();
		}
	}
	return bestMatch;
}

bool ServerConfig::isValidHost(const std::string& host) 
{
	struct in_addr addr;
	return inet_pton(AF_INET, host.c_str(), &addr) == 1;
}

void ServerConfig::setHost(const std::string& host) 
{
	if (host.empty()) 
	{
		_host = "0.0.0.0";
		return;
	}

	if (isValidHost(host)) 
	{
		_host = host;
		return;
	}

	// Otherwise, try to resolve the hostname to an IP address
	struct addrinfo hints, *res = NULL;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	int status = getaddrinfo(host.c_str(), NULL, &hints, &res);
	if (status != 0) 
	{
		throw std::runtime_error("Failed to resolve hostname '" + host + "': " + gai_strerror(status));
	}

	if (res != NULL) 
	{
		struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
		_host = inet_ntoa(ipv4->sin_addr);
		freeaddrinfo(res);
	} 
	else 
	{
		freeaddrinfo(res);
		throw std::runtime_error("No addresses found for host: " + host);
	}
}

void ServerConfig::setPort(const std::string& portStr) 
{
	if (portStr.empty()) 
		throw std::runtime_error("Port cannot be empty");

	for (size_t i = 0; i < portStr.length(); ++i) 
	{
		if (!isdigit(portStr[i])) 
			throw std::runtime_error("Port must contain only digits: " + portStr);
	}

	// Convert to integer and validate range
	int port = std::atoi(portStr.c_str());
	if (port < 1 || port > 65535) 
		throw std::runtime_error("Port out of range (1-65535): " + portStr);

	_ports.push_back(port);
}

void ServerConfig::setServerName(const std::string& name) 
{
	_serverName = name;
}

void ServerConfig::setClientMaxBodySize(const std::string& size) 
{
	_clientMaxBodySize = Utils::stringToSizeT(size);
}

void ServerConfig::setClientBodyTmpPath(const std::string& path) 
{
	_clientBodyTmpPath = path;
}

std::string ServerConfig::getClientBodyTmpPath() const 
{
	return _clientBodyTmpPath;
}

void ServerConfig::setErrorPage(const std::string& value) 
{
	std::istringstream iss(value);
	std::string codeStr;
	std::string path;

	if (!(iss >> codeStr)) 
		throw std::runtime_error("Missing error code in error_page directive");

	int code;
	std::istringstream codeStream(codeStr);
	if (!(codeStream >> code) || code < 100 || code > 599)
		throw std::runtime_error("Invalid HTTP error code: " + codeStr);

	if (!(iss >> path)) 
		throw std::runtime_error("Missing path in error_page directive");

	_errorPages[code] = path;
}

void ServerConfig::addLocation(const std::string& path, const LocationConfig& location) 
{
	_locations[path] = location;
}

const std::string& ServerConfig::getHost() const 
{
	return _host;
}

uint16_t ServerConfig::getPort() const 
{
	return _ports.empty() ? 0 : _ports[0];
}

const std::vector<uint16_t>& ServerConfig::getPorts() const 
{
	return _ports;
}

const std::string& ServerConfig::getServerName() const 
{
	return _serverName;
}

size_t ServerConfig::getClientMaxBodySize() const 
{
	return _clientMaxBodySize;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const 
{
	return _errorPages;
}

std::string ServerConfig::getErrorPage(int statusCode) const 
{
	std::string message = Utils::getMessage(statusCode);

	std::map<int, std::string>::const_iterator it = _errorPages.find(statusCode);
	if (it != _errorPages.end() && Utils::fileExists(it->second))
		return it->second;

	std::ostringstream oss;
	oss << "<!DOCTYPE html>\n<html>\n"
		<< "<head><title>" << statusCode << " " << message << "</title></head>\n"
		<< "<body>\n"
		<< "<h1><center>" << statusCode << " " << message << "</center></h1>\n"
		<< "<hr><center>Webserver 1337/1.0</center>\n"
		<< "</body>\n</html>\n";

	return oss.str();
}

int ServerConfig::getFd(size_t index) const 
{
	if (index >= _server_fds.size()) 
		throw std::runtime_error("File descriptor index out of range");

	return _server_fds[index];
}

const std::vector<int>& ServerConfig::getFds() const 
{
	return _server_fds;
}

const std::map<std::string, LocationConfig>& ServerConfig::getLocations() const 
{
	return _locations;
}

void ServerConfig::setupServer() 
{
	cleanupServer();

	for (std::vector<uint16_t>::const_iterator it = _ports.begin(); it != _ports.end(); ++it) 
	{
		try 
		{
			int fd = createSocket(*it);
			_server_fds.push_back(fd);
		} 
		catch (const std::exception& e) 
		{
			cleanupServer();
			throw; // Re-throw the exception
		}
	}
}

void ServerConfig::cleanupServer() 
{
	for (std::vector<int>::iterator it = _server_fds.begin(); it != _server_fds.end(); ++it) 
	{
		if (*it >= 0) 
			if (close(*it) == -1)
				std::cerr << "Warning: Failed to close socket: " << *it << " (errno: " << errno << ")" << std::endl;
	}
	_server_fds.clear();
	_server_addresses.clear();
}

int ServerConfig::createSocket(uint16_t port) 
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) 
		throw std::runtime_error(std::string("Failed to create socket: ") + strerror(errno));

	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) 
	{
		close(fd);
		throw std::runtime_error(std::string("Failed to set socket options: "));
	}

	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) 
	{
		int save_errno = errno;
		close(fd);
		throw std::runtime_error(std::string("Failed to set non-blocking mode: ") +  strerror(save_errno));
	}

	struct sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) == -1) 
	{
		int save_errno = errno;
		close(fd);
		throw std::runtime_error("Failed to bind socket on port " + Utils::toString(port) + ": " + strerror(save_errno));
	}

	if (listen(fd, SOMAXCONN) == -1) 
	{
		int save_errno = errno;
		close(fd);
		throw std::runtime_error("Failed to listen on socket on port " + Utils::toString(port) + ": " + strerror(save_errno));
	}
	_server_addresses.push_back(address);

	return fd;
}
#include "../include/ServerManager.hpp"
#include "../include/ServerConfig.hpp"
#include "../include/HTTPResponse.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Utils.hpp"
#include "../include/Logger.hpp"
#include "../include/Client.hpp"
#include "../include/SessionManager.hpp"
#include <cstddef>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>


ServerManager::ServerManager(const std::vector<ServerConfig>& servers) : _epollManager(MAX_EVENTS), _running(false), _clientTimeout(CLIENT_TIMEOUT), _maxClients(MAX_CLIENTS), _servers(servers)
{
	LOG_INFO("ServerManager initialized with timeout=" + Utils::toString(_clientTimeout) + "s, maxClients=" + Utils::toString(_maxClients));
}

ServerManager::~ServerManager()
{
	LOG_INFO("ServerManager shutting down");
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}
	_clients.clear();
}

bool ServerManager::init()
{
	for (size_t i = 0; i < _servers.size(); ++i)
	{
		try 
		{
			_servers[i].setupServer();
			const std::vector<int>& fds = _servers[i].getFds();
			const std::vector<uint16_t>& ports = _servers[i].getPorts();
			for (size_t j = 0; j < fds.size(); ++j)
			{
				_serverMap[fds[j]] = &_servers[i];
				if (!_epollManager.add(fds[j], EPOLLIN))
					return false;
				LOG_INFO("Server socket " + Utils::toString(fds[j]) + " listening on " + _servers[i].getHost() + ":" + Utils::toString(ports[j]));
			}
		}
		catch (const std::exception& e)
		{
			LOG_ERROR("Error setting up server: " + std::string(e.what()));
			return false;
		}
	}
	return true;
}

void ServerManager::run()
{
	_running = true;
	LOG_INFO("Starting server event loop");

	while (_running)
	{
		int numEvents = _epollManager.wait(1000);
		if (numEvents < 0)
		{
			if (errno == EINTR)
				continue;
			LOG_ERROR("Epoll wait error: " + std::string(std::strerror(errno)));
			break;
		}
		for (int i = 0; i < numEvents; ++i)
		{
			const epoll_event& event = _epollManager.getEvent(i);
			handleEvent(event);
		}
		checkClientTimeouts();
	}
	LOG_INFO("Server event loop stopped");
}

void ServerManager::stop()
{
	LOG_INFO("Stopping server");
	_running = false;
}

void ServerManager::handleEvent(const struct epoll_event& event)
{
	int fd = event.data.fd;
	uint32_t events = event.events;
	ServerConfig* server = findServerByFd(fd);

	if (server)
	{
		if (events & EPOLLIN)
			if (!acceptClient(fd, server))
				LOG_ERROR("Failed to accept client on server fd: " + Utils::toString(fd));
		return;
	}

	Client* client = findClientByFd(fd);
	if (!client)
	{
		_epollManager.remove(fd);
		return;
	}

	if (events & (EPOLLERR | EPOLLHUP))
	{
		cleanupClient(fd, (events & EPOLLERR) ? "Socket error" : "Connection hangup");
		return;
	}

	if (events & EPOLLIN)
	{
		if (client->isCgi())
			handleCgiResponse(client);
		else
		{
			if (!receiveFromClient(client))
				return(cleanupClient(fd, "Connection complete"));

			if (client->getRequest()->hasCgi() && client->getRequest()->isComplete())
				startCgi(client);
		}
	}
	else if ((events & EPOLLOUT))
	{
		if (!sendToClient(client))
			return(cleanupClient(fd, "Connection complete"));
	}
}


bool ServerManager::acceptClient(int serverFd, ServerConfig* serverConfig)
{
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen);
	if (clientFd == -1)
		return false;

	if (_clients.size() >= static_cast<size_t>(_maxClients))
	{
		LOG_WARN("Max clients reached (" + Utils::toString(_maxClients) + "), rejecting new connection");
		close(clientFd);
		return false;
	}

	int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags == -1 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		close(clientFd);
		return false;
	}
	try
	{
		Client* client = new Client(clientFd, serverConfig);
		_clients[clientFd] = client;
		if (!_epollManager.add(clientFd, EPOLLIN))
		{
			delete client;
			_clients.erase(clientFd);
			close(clientFd);
			return false;
		}
		std::string clientIp = inet_ntoa(clientAddr.sin_addr);
		int clientPort = ntohs(clientAddr.sin_port);
		LOG_INFO("New client connected from " + clientIp + ":" + Utils::toString(clientPort) + " (fd: " + Utils::toString(clientFd) + ", total: " + Utils::toString(_clients.size()) + ")");
		return true;
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Error creating client: " + std::string(e.what()));
		close(clientFd);
		return false;
	}
}

bool ServerManager::receiveFromClient(Client* client)
{
	if (!client)
		return false;

	char buffer[BUFFER_SIZE];
	std::memset(buffer, 0, BUFFER_SIZE);

	ssize_t bytesRead = recv(client->getFd(), buffer, sizeof(buffer) - 1, 0);
	if (bytesRead > 0)
	{
		client->updateActivity();
		client->setReadBuffer(buffer, bytesRead);
		client->getRequest()->parse(client->getReadBuffer());
		
		if (client->getRequest()->isComplete() && !client->getRequest()->hasCgi())
		{
			client->setSession(&SessionManager::get().acquire(client->getRequest(), client->getResponse()));

			// print the sid of all session in webserv
			SessionManager::get().printSession();
			// login logic
			

			if (LoginController::handle(client->getRequest(), client->getResponse(), *client->getSession()))
				client->getResponse()->buildHeader();
			else
				client->getResponse()->buildResponse();

			if (!_epollManager.modify(client->getFd(), EPOLLOUT))
				return false;
		}
		return true;
	}
	if (bytesRead == 0)
	{
		LOG_INFO("Client fd " + Utils::toString(client->getFd()) + " closed connection");
		return false;
	}
	LOG_ERROR("Error receiving from client fd " + Utils::toString(client->getFd()) + ": " + std::string(std::strerror(errno)));
	return false;
}

bool ServerManager::sendToClient(Client* client)
{
	if (!client)
		return false;

	char buffer[BUFFER_SIZE];
	ssize_t bytesToSend = client->getResponseChunk(buffer, sizeof(buffer));
	if (bytesToSend > 0)
	{
		ssize_t bytesSent = send(client->getFd(), buffer, bytesToSend, 0);
		if (bytesSent <= 0)
		{
			LOG_ERROR("Error sending to client fd " + Utils::toString(client->getFd()) + ": " + std::string(std::strerror(errno)));
			return false;
		}
		return true;
	}

	else if (bytesToSend == 0)
	{
		if (!client->shouldKeepAlive())
			return (LOG_INFO("closing connection for client fd " + Utils::toString(client->getFd())), false);

		if (!_epollManager.modify(client->getFd(), EPOLLIN))
			return false;
		client->reset();
		LOG_INFO("Keeping connection alive for client fd " + Utils::toString(client->getFd()));
		return true;
	}
	LOG_ERROR("Error generating response for client fd "+ Utils::toString(client->getFd()));
	return false;
}

void ServerManager::startCgi(Client* client)
{
	try
	{
		CGIHandler* cgiHandler = new CGIHandler(client->getRequest(), client->getResponse());
		cgiHandler->setEnv();
		cgiHandler->start();
		client->updateActivity();
		client->setCgiHandler(cgiHandler);
		client->setCgiStartTime(time(NULL));
		client->setIsCgi(true);

		_epollManager.add(cgiHandler->getPipeFd(), EPOLLIN);
		LOG_INFO("CGI process started for URI: " + client->getRequest()->getUri());
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Error starting CGI process: " + std::string(e.what()));
		cleanupClient(client->getFd(), "Failed to start CGI process");
	}
}

void ServerManager::handleCgiResponse(Client* client)
{
	if (!client || !client->isCgi())
		return;

	char buffer[BUFFER_SIZE] = {0};
	ssize_t bytesRead;

	try 
	{
		while ((bytesRead = read(client->getCgiHandler()->getPipeFd(), buffer, BUFFER_SIZE - 1)) > 0)
		{
			client->updateActivity();
			client->getResponse()->parseCgiOutput(std::string(buffer, bytesRead));
			std::memset(buffer, 0, BUFFER_SIZE);  // Clear buffer for next read
		}
		if (bytesRead <= 0) 
		{
			_epollManager.remove(client->getCgiHandler()->getPipeFd());
			client->getResponse()->buildResponse();
			client->setIsCgi(false);

			if (!_epollManager.modify(client->getFd(), EPOLLOUT))
				throw std::runtime_error("Failed to modify client to EPOLLOUT mode");

			LOG_INFO("CGI response completed for client fd: " + Utils::toString(client->getFd()));
		}
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Failed to handle CGI response: " + std::string(e.what()));
		cleanupClient(client->getFd(), "Error during CGI response handling: " + std::string(e.what()));
	}
}

void ServerManager::cleanupClient(int fd, const std::string& reason)
{
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it != _clients.end())
	{
		Client* client = it->second;
		if (client->isCgi() && client->getCgiHandler() != NULL)
		{
			_epollManager.remove(client->getCgiHandler()->getPipeFd());
			client->getCgiHandler()->cleanup();
			LOG_INFO("Cleaned up CGI process for client fd: " + Utils::toString(fd) + ", reason: " + reason);
		}

		LOG_INFO("Client disconnected (fd: " + Utils::toString(fd) + ", reason: " + reason + ", remaining: " + Utils::toString(_clients.size() - 1) + ")");
		delete it->second;
		_clients.erase(it);
	}

	_epollManager.remove(fd);
	close(fd);
}

void ServerManager::checkClientTimeouts()
{
	if (_clientTimeout <= 0)
		return;

	time_t currentTime = time(NULL);
	std::vector<int> timeoutFds;

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		Client* client = it->second;
		if (client->isCgi())
		{
			if ((currentTime - client->getCgiStartTime()) >= _clientTimeout)
				timeoutFds.push_back(it->first);
		}
		else
	{
			if ((currentTime - client->getLastActivity()) >= _clientTimeout)
				timeoutFds.push_back(it->first);
		}
	}

	for (size_t i = 0; i < timeoutFds.size(); ++i)
		cleanupClient(timeoutFds[i], "Connection timed out");

	if (!timeoutFds.empty())
		LOG_INFO("Cleaned up " + Utils::toString(timeoutFds.size()) + " client(s) due to " + Utils::toString(_clientTimeout) + "s timeout");

	// delete the expired session
	SessionManager::get().reapExpired(currentTime);
}

ServerConfig* ServerManager::findServerByFd(int fd)
{
	std::map<int, ServerConfig*>::iterator it = _serverMap.find(fd);
	if (it != _serverMap.end())
		return it->second;
	return NULL;
}

Client* ServerManager::findClientByFd(int fd)
{
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it != _clients.end())
		return it->second;

	for (std::map<int, Client*>::iterator cIt = _clients.begin(); cIt != _clients.end(); ++cIt)
	{
		Client* c = cIt->second;
		if (c->isCgi() && c->getCgiHandler() && c->getCgiHandler()->getPipeFd() == fd)
			return c;
	}
	return NULL;
}

void ServerManager::setClientTimeout(int seconds)
{
	_clientTimeout = seconds;
	LOG_INFO("Client timeout set to " + Utils::toString(seconds) + " seconds");
}

size_t ServerManager::getActiveClientCount() const
{
	return _clients.size();
}
#include "../include/SessionManager.hpp"

SessionManager::SessionManager() : _maxAge(1800) , _idleTimeout(11600) {}

SessionManager&	SessionManager::get()
{
	static SessionManager	instance;
	return instance;
}

Session&	SessionManager::acquire(HTTPRequest* req, HTTPResponse* res)
{
	std::string	sid = req->getCookie("sid");
    
	if ( _sessions.find(sid) == _sessions.end())
	{
		// generate new cookie and stuff
		sid = generateSessionID();
	
		Session newSession;
		newSession.created = std::time(NULL);
		newSession.lastAccess = std::time(NULL);
	
		_sessions[sid] = newSession;
	
		Cookie  c("sid", sid);
		c._sameSite = "Lax";
		c._httpOnly = true;
		c._path = "/";
		res->addCookie(c);
	}

	Session &session = _sessions[sid];
	session.lastAccess = std::time(NULL);
	return session;
}

void SessionManager::reapExpired(const time_t& now)
{
    std::map<std::string, Session>::iterator it = _sessions.begin();

    while (it != _sessions.end())
    {
        // keep a copy of the current element
        std::map<std::string, Session>::iterator cur = it++;

        if ( static_cast<size_t>(now - cur->second.created)     > _maxAge ||
             static_cast<size_t>(now - cur->second.lastAccess) > _idleTimeout )
        {
            _sessions.erase(cur);   // safe: 'cur' is still valid, 'it' already points to the next element
        }
    }
}



bool	SessionManager::isValidSessionID(const std::string& sid)
{
	size_t i = 0;
	while (i < sid.size())
	{
		if ( !std::isalnum(sid[i]) )
			return false;
		i++;
	}
	return true;
}

std::string	SessionManager::generateSessionID()
{
	std::string sid;
	for (int i = 0; i < 16; i++)
		sid += "0123456789abcdef"[std::rand() % 16];
	for (std::map<std::string, Session>::iterator it = _sessions.begin(); it != _sessions.end(); it++)
	{
		if (sid == it->first)
			generateSessionID();
	}
	return sid;
}

void SessionManager::printSession()
{
	std::cout << "---------------all-session-------------------\n";

	for (std::map<std::string, Session>::iterator it = _sessions.begin(); it != _sessions.end(); it++)
		std::cout << "sid: " << it->first << std::endl;
	
	std::cout << "---------------------------------------------\n";
}#include "../include/Utils.hpp"
#include <fstream>
#include <unistd.h>
#include <ctype.h>
#include <cstring>


std::string Utils::getMimeType(const std::string &path) // TODO 
{
	size_t dot_pos = path.find_last_of(".");
	if (dot_pos == std::string::npos)
		return "application/octet-stream";

	std::string extension = path.substr(dot_pos);
	if (extension == ".html" || extension == ".htm") 
		return "text/html";
	if (extension == ".css") 
		return "text/css";
	if (extension == ".js") return 
		"application/javascript";
	if (extension == ".json") 
		return "application/json";
	if (extension == ".png") 
		return "image/png";
	if (extension == ".jpg" || extension == ".jpeg") 
		return "image/jpeg";
	if (extension == ".gif") 
		return "image/gif";
	if (extension == ".svg") 
		return "image/svg+xml";
	if (extension == ".ico") 
		return "image/x-icon";
	if (extension == ".txt") 
		return "text/plain";
	if (extension == ".pdf") 
		return "application/pdf";
	return "application/octet-stream";
}

// Example of RFC 1123 date format: "Tue, 13 May 2025 23:58:00 GMT"
std::string Utils::getCurrentDate()
{
	char buffer[128];
	time_t now = time(NULL);
	struct tm* gmt = gmtime(&now);
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
	return std::string(buffer);
}

bool Utils::isPathWithinRoot(const std::string& root, const std::string& path) 
{
	return path.find(root) == 0;
}

std::string Utils::readFileContent(const std::string& filePath) 
{
	std::ifstream file(filePath.c_str(), std::ios::binary);
	if (!file.is_open()) 
		throw std::runtime_error("Could not open file: " + filePath);

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

bool Utils::isDirectory(const std::string& path)
{
	struct stat pathStat;
	if (stat(path.c_str(), &pathStat) != 0)
		return false;
	return S_ISDIR(pathStat.st_mode);
}

bool Utils::fileExists(const std::string& path) 
{
	struct stat fileInfo;

	if (stat(path.c_str(), &fileInfo))
		return false;
	return S_ISREG(fileInfo.st_mode);
}

std::string Utils::trim(const std::string& str) 
{
	size_t first = str.find_first_not_of(" \t");
	size_t last = str.find_last_not_of(" \t");
	if (first == std::string::npos || last == std::string::npos) 
		return "";
	return str.substr(first, last - first + 1);
}

void Utils::skipWhitespace(std::string& str) 
{
	size_t i = 0;
	while (i < str.size() && (str[i] == ' ' || str[i] == '\t')) 
		i++;
	str.erase(0, i);
}

std::string Utils::listDirectory(const std::string& dirPath, const std::string& root, const std::string& requestUri)
{
	if (!isPathWithinRoot(root, dirPath)) 
		return "<html><body><h1>403 Forbidden</h1></body></html>";

	DIR* dir = opendir(dirPath.c_str());
	if (dir == NULL) 
		return "<html><body><h1>404 Not Found</h1></body></html>";

	struct dirent* entry;
	std::vector<std::string> files;

	while ((entry = readdir(dir)) != NULL) 
	{
		std::string name(entry->d_name);
		if (name != "." && name != "..") 
			files.push_back(name);
	}
	closedir(dir);

	std::sort(files.begin(), files.end());

	std::stringstream body;
	body << "<html><head><title>Index of " << requestUri << "</title></head><body>";
	body << "<h1>Index of " << requestUri << "</h1>";
	body << "<ul>";

	if (requestUri != "/" && requestUri != "") 
	{
		std::string parent = requestUri;

		if (!parent.empty() && parent[parent.size() - 1] == '/')
			parent.erase(parent.size() - 1);

		size_t lastSlash = parent.find_last_of('/');
		if (lastSlash != std::string::npos) 
			parent = parent.substr(0, lastSlash + 1);
		else 
			parent = "/";
		body << "<li><a href=\"" << parent << "\">../</a></li>";
	}
	for (size_t i = 0; i < files.size(); ++i) 
	{
		const std::string& file = files[i];

		std::string fullPath = dirPath + "/" + file;
		struct stat s;
		std::string fileLink = requestUri;
		if (!fileLink.empty() && fileLink[fileLink.size() - 1] != '/')
			fileLink += "/";
		fileLink += file;

		if (stat(fullPath.c_str(), &s) == 0 && S_ISDIR(s.st_mode)) 
			body << "<li><a href=\"" << fileLink << "/\">" << file << "/</a></li>";
		else 
			body << "<li><a href=\"" << fileLink << "\">" << file << "</a></li>";
	}

	body << "</ul></body></html>";
	return body.str();
}

int Utils::urlDecode(std::string& uri) 
{
	std::string result;
	result.reserve(uri.length());

	for (size_t i = 0; i < uri.length(); ++i)
	{
		if (uri[i] == '%')
		{
			if (i + 2 >= uri.length())
				return -1;

			int value;
			std::istringstream iss(uri.substr(i + 1, 2));
			if (!(iss >> std::hex >> value))
				return -1;

			result += static_cast<char>(value);
			i += 2;
		}
		else if (uri[i] == '+')
			result += ' ';
		else
			result += uri[i];
	}
	uri = result;
	return 0;
}


bool strToSizeT(const std::string& str, size_t& result, bool allowZero = false) 
{
	if (str.empty()) 
		return (false);

	for (size_t i = 0; i < str.size(); ++i) 
		if (!isdigit(str[i])) 
			return false;

	std::stringstream ss(str);
	ss >> result;

	if (ss.fail() || !ss.eof()) 
		return (false);

	if (!allowZero && result == 0) 
		return (false);

	return (true);
}

std::string Utils::getMessage(int code) 
{
	switch(code) 
	{
		// Success
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No Content";
		case 205: return "Reset Content";
		case 206: return "Partial Content";
		case 207: return "Multi-Status";
		case 208: return "Already Reported";
		case 226: return "IM Used";

			  // Redirection
		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";
		case 305: return "Use Proxy";
		case 306: return "Switch Proxy";
		case 307: return "Temporary Redirect";
		case 308: return "Permanent Redirect";

			  // Client Errors
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 406: return "Not Acceptable";
		case 407: return "Proxy Authentication Required";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 410: return "Gone";
		case 411: return "Length Required";
		case 412: return "Precondition Failed";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 416: return "Range Not Satisfiable";
		case 417: return "Expectation Failed";
		case 418: return "I'm a teapot";
		case 421: return "Misdirected Request";
		case 422: return "Unprocessable Entity";
		case 423: return "Locked";
		case 424: return "Failed Dependency";
		case 425: return "Too Early";
		case 426: return "Upgrade Required";
		case 428: return "Precondition Required";
		case 429: return "Too Many Requests";
		case 431: return "Request Header Fields Too Large";
		case 451: return "Unavailable For Legal Reasons";

		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		case 505: return "HTTP Version Not Supported";
		case 506: return "Variant Also Negotiates";
		case 507: return "Insufficient Storage";
		case 508: return "Loop Detected";
		case 510: return "Not Extended";
		case 511: return "Network Authentication Required";

		default: return "Unknown Status";
	}
}

std::vector<std::string> Utils::listDirectory(const std::string& path)
{
	std::vector<std::string> entries;
	DIR* dir = opendir(path.c_str());
	if (!dir)
		return entries;

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (std::string(entry->d_name) == ".")
			continue;
		entries.push_back(std::string(entry->d_name));
	}
	closedir(dir);
	return entries;
}

size_t Utils::stringToSizeT(const std::string& str) 
{
	std::istringstream iss(str);
	size_t result;
	iss >> result;

	if (iss.fail()) 
		throw std::invalid_argument("Invalid size_t conversion: " + str);

	char remaining;
	if (iss >> remaining) 
		throw std::invalid_argument("Extra characters after number: " + str);
	return (result);
}
bool Utils::isValidMethodChar(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
           c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
           c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

bool Utils::isValidMethodToken(const std::string& method)
{
    if (method.empty())
        return false;

    // RFC 7230 defines tokens as consisting of visible ASCII characters
    // except for delimiters (like spaces, tabs, etc.)
    for (size_t i = 0; i < method.length(); i++) {
        char c = method[i];

        // Check for invalid ASCII characters (control characters or non-printable characters)
        if (c <= 32 || c >= 127 || 
            c == '(' || c == ')' || c == '<' || c == '>' || 
            c == '@' || c == ',' || c == ';' || c == ':' || 
            c == '\\' || c == '"' || c == '/' || c == '[' || 
            c == ']' || c == '?' || c == '=' || c == '{' || 
            c == '}' || c == ' ' || c == '\t')
            return false;

        // Check if the character is a lowercase letter (invalid for HTTP method)
        if (c >= 'a' && c <= 'z') {
            return false;  // Reject lowercase letters
        }
    }

    return true;
}

bool Utils::isValidVersion(const std::string& version)
{
	// Basic length check
	if (version.length() != 8)  // "HTTP/1.1" is exactly 8 chars
		return false;

	// Check prefix and format
	return (version.substr(0, 5) == "HTTP/" && 
	isdigit(version[5]) && 
	version[6] == '.' && 
	isdigit(version[7]));
}


size_t Utils::skipLeadingWhitespace(const std::string& str)
{
	size_t pos = 0;
	while (pos < str.size() && (str[pos] == ' ' || str[pos] == '\t')) 
		++pos;
	return pos;
}

bool Utils::isValidUri(const std::string& uri)
{
    for (size_t i = 0; i < uri.size(); ++i) 
        if (!std::isprint(static_cast<unsigned char>(uri[i]))) 
            return false;
    return true;
}

bool Utils::isSupportedMethod(const std::string& method)
{
	return (method == "GET" || method == "POST" || method == "DELETE");
}

// Validate HTTP header key as a token per RFC 7230 (tchar characters)
bool Utils::isValidHeaderKey(const std::string& key) 
{
	if (key.empty()) 
		return false;

	for (size_t i = 0; i < key.size(); ++i) 
	{
		char c = key[i];
		if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			  (c >= '0' && c <= '9') || c == '!' || c == '#' ||
			  c == '$' || c == '%' || c == '&' || c == '\'' ||
			  c == '*' || c == '+' || c == '-' || c == '.' ||
			  c == '^' || c == '_' || c == '`' || c == '|' ||
			  c == '~')) 
		{
			return false;
		}
	}
	return true;
}


// Validate HTTP header value per RFC 7230 (field-value)
bool Utils::isValidHeaderValue(const std::string& value) 
{
	// Empty values are allowed in some cases (e.g., Host: )
	if (value.empty()) 
		return true;

	// Check each character against field-vchar (VCHAR) and SP/HTAB
	for (size_t i = 0; i < value.size(); ++i) 
		if (value[i] != ' ' && value[i] != '\t' && (value[i] < 33 || value[i] > 126)) 
			return false;
	return true;
}

std::string Utils::extractAttribute(const std::string& headers, const std::string& key) {
    std::string search = key + "=\"";
    size_t start = headers.find(search);
    if (start == std::string::npos) return "";
    start += search.length();
    size_t end = headers.find("\"", start);
    if (end == std::string::npos) return "";
    return headers.substr(start, end - start);
}

std::string Utils::getFileExtension(const std::string& path)
{
	size_t dotPos = path.find_last_of('.');
	if (dotPos != std::string::npos) 
		return path.substr(dotPos);
	return "";
}


bool Utils::isExecutable(const std::string& path)
{
	return access(path.c_str(), X_OK) == 0;
}


char** Utils::mapToEnvp(const std::map<std::string, std::string>& env)
{
	std::vector<std::string> envStrings;

	for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it) 
		envStrings.push_back(it->first + "=" + it->second);

	char** result = new char*[envStrings.size() + 1];

	for (size_t i = 0; i < envStrings.size(); ++i) 
	{
		result[i] = new char[envStrings[i].size() + 1];
		strcpy(result[i], envStrings[i].c_str());
	}

	result[envStrings.size()] = NULL;
	return result;
}


std::string Utils::createTempFile(const std::string& prefix, const std::string& dir)
{
	struct stat st;
	if (stat(dir.c_str(), &st) == -1)
	{
		if (errno == ENOENT)
		{
			if (mkdir(dir.c_str(), 0755) == -1)
				throw std::runtime_error("Failed to create directory: " + dir + " error: " + std::string(strerror(errno)));
		}
		else
			throw std::runtime_error("Failed to check directory: " + dir + " error: " + std::string(strerror(errno)));
	}

	std::stringstream ss;
	ss << dir << "/" << prefix << "_" << getpid() << "_" << time(NULL);
	std::string tempFile = ss.str();

	return tempFile;
}

// cookies :
std::map<std::string,std::string> Utils::parseCookieHeader(const std::string& h)
{
    std::map<std::string,std::string> res;
    std::stringstream ss(h);
    std::string token;
    while (std::getline(ss, token, ';'))
	{
        size_t eq = token.find('=');
        if (eq==std::string::npos) continue;
        std::string k = trim(token.substr(0,eq));
        std::string v = trim(token.substr(eq+1));
        res[k]=v;
    }
    return res;
}
