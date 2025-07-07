#include "../include/Epoll.hpp"
#include <stdexcept>
#include <cstring>
#include <errno.h>
#include <unistd.h>

Epoll::Epoll(size_t maxEvents) : _epollFd(-1) , _events(maxEvents)
{
	_epollFd = epoll_create(1);
	if (_epollFd == -1)
		throw std::runtime_error(std::string("Failed to create epoll: ") + strerror(errno));
}

Epoll::~Epoll()
{
	if (_epollFd != -1)
	{
		close(_epollFd);
		_epollFd = -1;
	}
}

bool Epoll::add(int fd, uint32_t events)
{
	if (_epollFd == -1 || fd < 0)
		return false;

	epoll_event ev = {};
	ev.events = events;
	ev.data.fd = fd;

	return (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) != -1);
}

bool Epoll::modify(int fd, uint32_t events)
{
	if (_epollFd == -1 || fd < 0)
		return false;

	epoll_event ev = {};
	ev.events = events;
	ev.data.fd = fd;

	return (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) != -1);
}

int Epoll::wait(int timeout)
{
	if (_epollFd == -1)
		return -1;
	return epoll_wait(_epollFd, _events.data(), _events.size(), timeout);
}

bool Epoll::remove(int fd)
{
	if (_epollFd == -1 || fd < 0)
		return false;
	return (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) != -1);
}


int Epoll::getFd() const
{
	return _epollFd;
}

const epoll_event& Epoll::getEvent(size_t index) const
{
	if (index >= _events.size())
		throw std::out_of_range("Epoll: Event index out of range");
	return _events[index];
}
