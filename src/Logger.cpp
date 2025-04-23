/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/09 21:51:45 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/10 14:34:11 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Logger.hpp"

Logger& Logger::getInstance() 
{
    static Logger instance;
    return instance;
}

Logger::Logger() : _currentLevel(INFO), _colorsEnabled(true)
{
}

void Logger::setLogLevel(LogLevel level) 
{
    _currentLevel = level;
}

void Logger::enableColors(bool enable) 
{
    _colorsEnabled = enable;
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

    std::cerr << (_colorsEnabled ? COLOR_TIME : "") << "[" << getTimestamp() << "] " << (_colorsEnabled ? COLOR_RESET : "");
    
    switch(level)
    {
        case DEBUG:
            std::cerr << (_colorsEnabled ? COLOR_DEBUG : "") << "[DEBUG] ";
            break;
        case INFO:
            std::cerr << (_colorsEnabled ? COLOR_INFO : "") << "[INFO] ";
            break;
        case WARNING:
            std::cerr << (_colorsEnabled ? COLOR_WARNING : "") << "[WARNING] ";
            break;
        case ERROR:
            std::cerr << (_colorsEnabled ? COLOR_ERROR : "") << "[ERROR] ";
            break;
        case FATAL:
            std::cerr << (_colorsEnabled ? COLOR_FATAL : "") << "[FATAL] ";
            break;
    }
    
    std::cerr << message << (_colorsEnabled ? COLOR_RESET : "") << std::endl;
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
