#ifndef EPOLLMANAGER_HPP
#define EPOLLMANAGER_HPP

#include <sys/epoll.h>
#include <vector>

class Epoll 
{
public:
	Epoll(size_t maxEvents);
	~Epoll();

	Epoll(const Epoll&);
	Epoll& operator=(const Epoll&);

	bool add(int fd, uint32_t events);
	bool modify(int fd, uint32_t events);
	bool remove(int fd);
	int wait(int timeout = -1);
	const epoll_event& getEvent(size_t index) const;
	int getFd() const;

private:
	int _epollFd;
	std::vector<epoll_event> _events;
};

#endif
