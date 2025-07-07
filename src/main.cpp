#include "../include/ServerManager.hpp"
#include "../include/Logger.hpp"

ServerManager* globalServer = NULL;

void signal_handler(int signum) 
{
	(void) signum;
	if (globalServer) 
		globalServer->stop();
}

int main(int argc, char* argv[])
{
	signal(SIGINT, signal_handler);
	try 
	{
		std::string configFile = "./config/default.conf";
		if (argc > 1)
			configFile = argv[1];

		ServerConfig config(configFile);
		ServerManager serverManager(config.getServers());
		globalServer = &serverManager;

		if (!serverManager.init()) 
			throw std::runtime_error("Failed to initialize server.");

		serverManager.run();
	} 
	catch (const std::exception& e) 
	{
		LOG_ERROR(std::string(e.what()));
		return 1;
	}
	return (0);
}
