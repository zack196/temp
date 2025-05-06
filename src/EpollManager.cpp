/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollManager.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mregrag <mregrag@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/10 15:05:56 by mregrag           #+#    #+#             */
/*   Updated: 2025/04/22 17:34:54 by mregrag          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/webserver.hpp"

EpollManager::EpollManager()
{
	_epollFd = epoll_create(1);
	if (_epollFd == -1)
		throw std::runtime_error("Failed to create epoll instance");
}

EpollManager::~EpollManager() 
{
	close(_epollFd);
}

void EpollManager::addFd(int fd, uint32_t events)
{
	struct epoll_event event;
	memset(&event, 0, sizeof(event));
	event.data.fd = fd;
	event.events = events;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &event) == -1)
		throw std::runtime_error("Failed to add FD to epoll");
}

void EpollManager::modifyFd(int fd, uint32_t events)
{
	struct epoll_event event;
	memset(&event, 0, sizeof(event));
	event.data.fd = fd;
	event.events = events;
	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &event) == -1)
		throw std::runtime_error("Failed to modify FD in epoll");
}

void EpollManager::removeFd(int fd)
{
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
		throw std::runtime_error("Failed to remove FD from epoll");
}

int EpollManager::wait(std::vector<struct epoll_event>& events, int timeout)
{
	if (events.empty())
		return 0;

	int numEvents = epoll_wait(_epollFd, &events[0], events.size(), timeout);
	if (numEvents == -1)
		throw std::runtime_error("Epoll wait failed");
	return numEvents;
}
