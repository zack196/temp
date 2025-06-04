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
}