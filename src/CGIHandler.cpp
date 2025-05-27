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
