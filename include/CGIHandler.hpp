#ifndef S_CGI_HANDLER_HPP
#define S_CGI_HANDLER_HPP

#include <string>
#include <map>
#include <ctime>
#include <signal.h>
#include <sys/types.h> 
#include "HTTPRequest.hpp"


class CGIHandler
{
public:
	CGIHandler();
	~CGIHandler();

	void init(HTTPRequest* request);
	void start();

	bool isRunning(int& status);
	void killProcess();
	bool hasTimedOut();

	time_t getStartTime() const;
	std::string getOutputFile() const;
	pid_t getPid() const;

	void validatePaths() const;
	void buildEnv();
	void buildArgv();
	void cleanEnv();
	void cleanArgv();
	void cleanup();

private:
	pid_t           _pid;
	int             _ouFd;
	int             _inFd;

	std::string     _execPath;
	std::string     _scriptPath;
	std::string     _outputFile;
	std::string     _extension;

	char**          _envp;
	char**          _argv;

	std::map<std::string, std::string> _env;

	time_t          _startTime;

};

#endif

