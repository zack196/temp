#include "../include/SessionManager.hpp"

SessionManager::SessionManager(long ttlSeconds ) : _timeout(ttlSeconds) {}

std::string SessionManager::generateSID() 
{
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) // 128-bit SID
        ss << std::hex << std::setw(2) << std::setfill('0') << (rand() % 256);
    return ss.str();
}

SessionData::SessionData(std::string username, time_t lastSeen)
    : _username(username), _lastSeen(lastSeen)  {}

std::string SessionManager::createSession(const std::string &username)
{
    std::string sid;
    do
    {
        sid = generateSID();
    } while (_sessions.find(sid) != _sessions.end());
    _sessions[sid] = SessionData(username, time(NULL));
    return sid;
}

bool SessionManager::hasSession(const std::string& sid) const
{
    return _sessions.find(sid) != _sessions.end();
}

SessionData* SessionManager::getSession(const std::string& sid)
{
    std::map< std::string, SessionData >::iterator it = _sessions.find(sid);
    if (it != _sessions.end())
    {
        it->second._lastSeen = std::time(NULL);
        return &it->second;
    }
    return NULL;
}

void SessionManager::removeSession(const std::string& sid)
{
    _sessions.erase(sid);
}

void SessionManager::sweepExpiredSessions()
{
    std::map<std::string, SessionData>::iterator it = _sessions.begin();
    time_t  now = std::time(NULL);

    while (it != _sessions.end())
    {
        if (now - it->second._lastSeen > _timeout)
            _sessions.erase(it++);
        else
            ++it;
    }
}