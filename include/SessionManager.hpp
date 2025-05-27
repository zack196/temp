#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include <iostream>
#include <string>
#include <map>
#include <cstdlib>      // for rand()
#include <sstream>      // for stringstream
#include <iomanip>      // for hex
#include <algorithm>    // for std::remove_if
// SessionManager.hpp
struct SessionData
{
    std::string _username;
    time_t      _lastSeen;
    SessionData(std::string username, time_t lastSeen);
};

class SessionManager 
{
    public:
        SessionManager(long ttlSeconds = 30 * 60);   // optional default TTL
        std::string createSession(const std::string& username);
        bool hasSession(const std::string& sid) const;
        SessionData* getSession(const std::string& sid);
        void removeSession(const std::string& sid);
        void sweepExpiredSessions();                 // run every N seconds
    private:
        std::map<std::string, SessionData> _sessions;
        long _timeout;
    private:
        std::string generateSID();
};


#endif