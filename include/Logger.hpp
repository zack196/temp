#ifndef LOGGER_HPP
#define LOGGER_HPP

#define COLOR_RESET   "\033[0m"
#define COLOR_DEBUG   "\033[36m"
#define COLOR_INFO    "\033[32m"
#define COLOR_WARNING "\033[33m"
#define COLOR_ERROR   "\033[31m"
#define COLOR_FATAL   "\033[1;31m"
#define COLOR_TIME    "\033[90m"

#define LOG_DEBUG(msg)   Logger::getInstance().debug(msg)
#define LOG_INFO(msg)    Logger::getInstance().info(msg)
#define LOG_WARN(msg) Logger::getInstance().warning(msg)
#define LOG_ERROR(msg)   Logger::getInstance().error(msg)
#define LOG_FATAL(msg)   Logger::getInstance().fatal(msg)

#include <string>
#include <iostream>
#include <ctime>

class Logger 
{
	public:
		enum LogLevel 
		{
			DEBUG,
			INFO,
			WARNING,
			ERROR,
			FATAL
		};

		static Logger& getInstance();

		void debug(const std::string& message);
		void info(const std::string& message);
		void warning(const std::string& message);
		void error(const std::string& message);
		void fatal(const std::string& message);

	private:
		Logger();
		Logger(const Logger&);
		Logger& operator=(const Logger&);

		void log(LogLevel level, const std::string& message);
		std::string getTimestamp();
		std::string getLevelString(LogLevel level);
		LogLevel _currentLevel;
};


#endif

