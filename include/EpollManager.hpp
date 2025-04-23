#ifndef EPOLLMANAGER_HPP
#define EPOLLMANAGER_HPP

#include <vector>
#include <sys/epoll.h>
#include <stdexcept>
#include <unistd.h>
#include <cstring>

class EpollManager 
{
	public:
		EpollManager();
		~EpollManager();

		void addFd(int fd, uint32_t events);   // Register a socket for monitoring
		void modifyFd(int fd, uint32_t events); // Change monitored events
		void removeFd(int fd);                 // Stop monitoring an FD
		int wait(std::vector<struct epoll_event>& events, int timeout); // Wait for events

	private:
		int _epollFd; // File descriptor for the epoll instance
};

#endif
