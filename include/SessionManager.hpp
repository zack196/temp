#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include <ctime>


struct Session 
{
    std::map<std::string, std::string> data;
    time_t created;
    time_t lastAccess;
};

class SessionManager
{
    public:
        static SessionManager& get();

        Session& acquire(HTTPRequest* req, HTTPResponse* res);
        void reapExpired(const time_t& now);
        void printSession();

    private:
        SessionManager();
        std::string generateSessionID();
        bool isValidSessionID(const std::string& sid);

        std::map<std::string, Session> _sessions;
        size_t _maxAge;         // total lifetime in seconds (e.g. 12h)
        size_t _idleTimeout;    // idle timeout in seconds (e.g. 30min)
};

#endif