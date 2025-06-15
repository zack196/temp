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

	if (path == "/login" && methode == "GET") 
	{
		if (sess.data.count("user")) 
		{		// already logged in
			res->setStatusCode(302);
			res->setStatusMessage("Found");
			res->setHeader("Location", "/");
			res->setHeader("Content-Length", "0");
			return true;						// tell caller we handled it
		}
		return false;							// show login.html
	}

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
		

		if (validUser(user, pass))
		{
			sess.data["user"] = user;
			res->setStatusCode(302);
			res->setStatusMessage("Found");
			res->setHeader("Location", "/");
		}
		else
		{
			std::cout << "in\n";
			res->setStatusCode(200);
			res->setStatusMessage("OK");
			res->setHeader("Content-Type","text/html");
			res->appendToBody("<!DOCTYPE html>"
						"<html lang=\"en\">"
						"<head>"
						"    <meta charset=\"UTF-8\">"
						"    <title>Login Error</title>"
						"    <style>"
						"        body {"
						"            font-family: Arial, sans-serif;"
						"            background-color: #f8d7da;"
						"            color: #721c24;"
						"            display: flex;"
						"            justify-content: center;"
						"            align-items: center;"
						"            height: 100vh;"
						"            margin: 0;"
						"        }"
							""
						"        .error-box {"
						"            background-color: #f5c6cb;"
						"            border: 1px solid #f1b0b7;"
						"            padding: 30px 50px;"
						"            border-radius: 10px;"
						"            text-align: center;"
						"        }"
							""
						"        h1 {"
						"            margin: 0 0 10px;"
						"        }"
							""
						"        p {"
						"            margin: 0 0 20px;"
						"        }"
							""
						"        a {"
						"            color: #721c24;"
						"            text-decoration: underline;"
						"        }"
							""
						"        a:hover {"
						"            text-decoration: none;"
						"        }"
						"    </style>"
						"</head>"
						"<body>"
						"    <div class=\"error-box\">"
						"        <h1>Login Failed</h1>"
						"        <p>Invalid username or password. Please try again.</p>"
						"        <a href=\"login.html\">Back to Login</a>"
						"    </div>"
						"</body>"
						"</html>" );
			res->setHeader("Content-Length", Utils::toString(res->getBody().size()));
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
