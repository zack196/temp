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

#endif