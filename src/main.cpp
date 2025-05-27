#include "../include/ConfigParser.hpp"
#include "../include/ServerManager.hpp"
#include "../include/Logger.hpp"
#include <signal.h>

ServerManager* globalServer = NULL;

void signal_handler(int signum) 
{
	if (globalServer) 
	{
		LOG_INFO("Signal " + Utils::toString(signum) + " received. Stopping server gracefully...");
		globalServer->stop();
	}
}


int main(int argc, char* argv[])
{
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	try 
	{
		std::string configFile = "./config/default.conf";
		if (argc > 1)
			configFile = argv[1];

		ConfigParser parser(configFile);
		parser.parseFile();

		std::vector<ServerConfig> servers = parser.getServers();
		if (servers.empty()) 
		{
			LOG_ERROR("No valid server configurations found");
			return 1;
		}

		ServerManager serverManager(servers);
		globalServer = &serverManager;

		if (!serverManager.init()) 
		{
			LOG_ERROR("Failed to initialize server");
			return 1;
		}

		serverManager.run();
		return 0;
		
	} 
	catch (const std::exception& e) 
	{
		LOG_ERROR("Fatal error: " + std::string(e.what()));
		return 1;
	}
}
