#include "../include/Cookie.hpp"

Cookie::Cookie(const std::string& name, const std::string& value) 
    :_name(name), _value(value), _expires(0), _secure(false), _httpOnly(false) {}


static std::string formatExpires(time_t t) 
{
    // Format: Wdy, DD Mon YYYY HH:MM:SS GMT
    char buf[100];
    struct tm *gmt = gmtime(&t);
    if (gmt) {
        strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
        return std::string(buf);
    }
    return "";
}

std::string Cookie::toSetCookieString() const 
{
    std::ostringstream  oss;
    oss << _name << "=" << _value;
    if (!_path.empty()) 
        oss << "; Path=" << _path;
    if (!_domain.empty())
        oss << "; Domain=" << _domain;
    if (_expires != 0)
        oss << "; Expires=" << formatExpires(_expires);
    if (_secure)
        oss << "; Secure";
    if (_httpOnly)
        oss << "; HttpOnly";
    if (!_sameSite.empty())
        oss << "; SameSite=" << _sameSite;
    return oss.str();
}
