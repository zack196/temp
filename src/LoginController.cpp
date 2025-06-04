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
