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
