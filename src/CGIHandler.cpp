#include "../include/CGIHandler.hpp"
#include "../include/HTTPRequest.hpp"
#include <cstring>
#include <sys/stat.h>
#include <wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <fcntl.h>
#include <cerrno>
#include <fcntl.h>
#include <stdlib.h>
#include <iostream>


CGIHandler::CGIHandler()
	: _pid(-1),
	_ouFd(-1),
	_inFd(-1),
	_execPath(),
	_scriptPath(),
	_outputFile(),
	_envp(NULL),
	_argv(NULL),
	_env(),
	_startTime(0)
{
}

void CGIHandler::init(HTTPRequest* request)
{
	_scriptPath = request->getResource();
	_extension = Utils::getExtension(_scriptPath);
	_execPath = request->getLocation().getCgiPath(_extension);

	validatePaths();

	_outputFile = Utils::createTempFile("cgi_output_", request->getServer().getClientBodyTmpPath());

	_ouFd = open(_outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0600);
	if (_ouFd == -1)
		throw std::runtime_error("Failed to create output file: " + _outputFile + " error: " + std::string(strerror(errno)));

	_inFd = open(request->getBodyfile().c_str(), O_RDONLY);
	if (_inFd == -1)
		throw std::runtime_error("Failed to open request body file: " + std::string(strerror(errno)));


	_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	_env["SERVER_SOFTWARE"] = "Webserv/1.0";
	_env["REQUEST_METHOD"] = request->getMethod();
	_env["SCRIPT_NAME"] = _scriptPath;
	_env["SCRIPT_FILENAME"] = _scriptPath;
	_env["PATH_INFO"] = request->getPath();
	_env["PATH_TRANSLATED"] = _scriptPath;
	_env["QUERY_STRING"] = request->getQuery();
	_env["SERVER_PROTOCOL"] = request->getProtocol();
	_env["HTTP_HOST"] = request->getHeader("host");
	_env["CONTENT_LENGTH"] = Utils::toString(request->getContentLength());
	_env["CONTENT_TYPE"] = request->getHeader("content-type");
	_env["PYTHONIOENCODING"] = "utf-8";

	const std::map<std::string, std::string>& headers = request->getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		if (it->first == "host" || it->first == "content-type")
			continue;

		std::string envName = "HTTP_";
		for (std::string::size_type i = 0; i < it->first.size(); ++i)
		{
			char c = it->first[i];
			envName += (c == '-') ? '_' : static_cast<char>(toupper(c));
		}
		_env[envName] = it->second;
	}

}


CGIHandler::~CGIHandler()
{
	cleanup();
}



void CGIHandler::validatePaths() const
{
	if (_execPath.empty() || _scriptPath.empty())
		throw std::invalid_argument("Empty CGI paths");

	if (access(_execPath.c_str(), F_OK) != 0)
		throw std::invalid_argument("File not found");

	if (access(_execPath.c_str(), X_OK) != 0 || access(_scriptPath.c_str(), R_OK) != 0)
		throw std::runtime_error("Invalid Permissions");
}


void CGIHandler::buildEnv()
{
	cleanEnv();
	_envp = new char*[_env.size() + 1];
	int index = 0;
	for (std::map<std::string, std::string>::const_iterator it = _env.begin(); it != _env.end(); ++it)
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

time_t CGIHandler::getStartTime() const 
{
	return _startTime;
}

bool CGIHandler::isRunning(int& status)
{
	if (_pid <= 0)
		return false;
	pid_t result = waitpid(_pid, &status, WNOHANG);
	if (result == _pid)
	{
		_pid = -1;
		return false;
	}
	if (result == 0)
	{
		return true;
	}
	return false;
}


void CGIHandler::killProcess()
{
	if (_pid > 0) 
	{
		kill(_pid, SIGKILL);
		waitpid(_pid, NULL, 0); 
		_pid = -1;
	}
}


bool CGIHandler::hasTimedOut()
{
	if (_pid <= 0)
		return false;

	time_t now = time(NULL);
	return (now - _startTime) > CGI_TIMEOUT;
}


void CGIHandler::start()
{
	buildArgv();
	buildEnv();
	_startTime = time(NULL);


	_pid = fork();
	if (_pid < 0) 
	{
		close(_ouFd);
		throw std::runtime_error("Fork failed: " + std::string(strerror(errno)));
	}

	if (_pid == 0) 
	{
		if (dup2(_inFd, STDIN_FILENO) == -1) 
		{
			close(_inFd);
			close(_ouFd);
			exit(EXIT_FAILURE);
		}
		close(_inFd);
		if (dup2(_ouFd, STDOUT_FILENO) == -1) 
		{
			close(_ouFd);
			exit(EXIT_FAILURE);
		}

		if (dup2(_ouFd, STDERR_FILENO) == -1) 
		{
			close(_ouFd);
			exit(EXIT_FAILURE);
		}
		close(_ouFd);

		execve(_execPath.c_str(), _argv, _envp);
		exit(EXIT_FAILURE);
	}
}


std::string CGIHandler::getOutputFile() const
{
	return _outputFile;
}

pid_t CGIHandler::getPid() const
{
	return _pid;
}

void CGIHandler::cleanup()
{

	if (_ouFd != -1) 
	{
		close(_ouFd);
		_ouFd = -1;
	}
	if (_inFd != -1) 
	{
		close(_inFd);
		_inFd = -1;
	}
	if (!_outputFile.empty()) 
		std::remove(_outputFile.c_str());
	cleanEnv();
	cleanArgv();
}
