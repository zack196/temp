#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <map>
#include <vector>
#include <string>
#include "EpollManager.hpp"
#include "ServerConfig.hpp"
#include "Client.hpp"

class ServerManager 
{
public:
	// Constructs the ServerManager with a list of server configurations.
	// The timeout, maxClients, and maxEvents default to macros defined in Config.hpp.
	// Note: The caller only provides the servers vector.
	ServerManager(const std::vector<ServerConfig>& servers);

	~ServerManager();

	// Initializes the servers (sets them up and registers their sockets with epoll).
	void startCgi(Client *client);
	void handleCgiResponse(Client *client);
	bool init();

	// Runs the main event loop.
	void run();

	// Stops the event loop.
	void stop();
	void shutdown();

	// Sets the client timeout value (overriding the default macro value).
	void setClientTimeout(int seconds);

	// Returns the count of active client connections.
	size_t getActiveClientCount() const;

private:
	// Looks up the server configuration that owns the given fd.
	ServerConfig* findServerByFd(int fd);
	Client* findClientByFd(int fd);

	// Accepts a new client connection from the given server socket.
	bool acceptClient(int serverFd, ServerConfig* serverConfig);

	// Handles events for a given client.
	void handleClient(int fd, uint32_t events);
	void handleEvent(const epoll_event& event);

	// Reads data from the client.
	bool receiveFromClient(Client* client);

	// Sends response data to the client.
	bool sendToClient(Client* client);

	// Cleans up an individual client.
	void cleanupClient(int fd, const std::string& reason);

	// Periodically checks for and removes timed-out clients.
	void checkClientTimeouts();

	EpollManager _epollManager;
	bool         _running;
	int          _clientTimeout;
	int          _maxClients;
	int _epollFd;

	// Stored server configurations (as provided in the constructor).
	std::vector<ServerConfig> _servers;

	// A map of server socket file descriptor to its ServerConfig.
	std::map<int, ServerConfig*> _serverMap;

	// Active client connections mapped by their file descriptor.
	std::map<int, Client*> _clients;

	std::string _buffer;
	SessionManager    _sessionManager;
};

#endif // SERVERMANAGER_HPP
