#include "../include/EpollManager.hpp"
#include <stdexcept>
#include <cstring>
#include <errno.h>
#include <unistd.h>

EpollManager::EpollManager(size_t maxEvents) : _epollFd(-1) , _events(maxEvents)
{
	_epollFd = epoll_create(1);
	if (_epollFd == -1)
		throw std::runtime_error(std::string("Failed to create epoll: ") + strerror(errno));
}

EpollManager::~EpollManager()
{
	if (_epollFd != -1)
	{
		close(_epollFd);
		_epollFd = -1;
	}
}

bool EpollManager::add(int fd, uint32_t events)
{
	if (_epollFd == -1 || fd < 0)
		return false;

	epoll_event ev = {};
	ev.events = events;
	ev.data.fd = fd;

	return (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) != -1);
}

bool EpollManager::modify(int fd, uint32_t events)
{
	if (_epollFd == -1 || fd < 0)
		return false;

	epoll_event ev = {};
	ev.events = events;
	ev.data.fd = fd;

	return (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) != -1);
}

bool EpollManager::remove(int fd)
{
	if (_epollFd == -1 || fd < 0)
		return false;

	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
		if (errno != EBADF && errno != ENOENT)
			return false;

	return true;
}

int EpollManager::getFd() const
{
	return _epollFd;
}


int EpollManager::wait(int timeout)
{
	if (_epollFd == -1)
		return -1;

	int numEvents = epoll_wait(_epollFd, _events.data(), _events.size(), timeout);

	if (numEvents == -1 && errno != EINTR)
		throw std::runtime_error(std::string("epoll_wait failed: ") + 
			   strerror(errno));

	return numEvents;
}

const epoll_event& EpollManager::getEvent(size_t index) const
{
	if (index >= _events.size())
		throw std::out_of_range("EpollManager: Event index out of range");
	return _events[index];
}
