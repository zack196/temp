#include "../include/Logger.hpp"

Logger::Logger() : _currentLevel(INFO)
{
}

Logger& Logger::getInstance() 
{
    static Logger instance;
    return instance;
}

void Logger::debug(const std::string& message) 
{
    log(DEBUG, message);
}

void Logger::info(const std::string& message)
{
    log(INFO, message);
}

void Logger::warning(const std::string& message)
{
    log(WARNING, message);
}

void Logger::error(const std::string& message)
{
    log(ERROR, message);
}

void Logger::fatal(const std::string& message)
{
    log(FATAL, message);
}

void Logger::log(LogLevel level, const std::string& message)
{

    std::cout << COLOR_TIME  << "[" << getTimestamp() << "] " << COLOR_RESET;
    
    switch(level)
    {
        case DEBUG:
            std::cout << COLOR_DEBUG;
            break;
        case INFO:
            std::cout << COLOR_INFO;
            break;
        case WARNING:
            std::cout << COLOR_WARNING;
            break;
        case ERROR:
            std::cout << COLOR_ERROR;
            break;
        case FATAL:
            std::cout << COLOR_FATAL;
            break;
    }
    
    std::cout << message << COLOR_RESET << std::endl;
}

std::string Logger::getTimestamp() 
{
    time_t now;
    time(&now);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

std::string Logger::getLevelString(LogLevel level)
{
    switch(level)
    {
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        case FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}
