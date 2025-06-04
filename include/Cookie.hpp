#ifndef COOKIE_HPP
#define COOKIE_HPP

#include <iostream>
#include <string>
#include <sstream>


class Cookie
{
    private:
    public :
        std::string     _name;
        std::string     _value;
        std::string     _path;
        std::string     _domain;
        time_t          _expires;
        bool            _secure;
        bool            _httpOnly;
        std::string     _sameSite;
        
        Cookie(const std::string& name, const std::string& value);
        std::string toSetCookieString() const;
};


#endif