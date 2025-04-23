#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include "EpollManager.hpp"
#include "ServerConfig.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include "ServerConfig.hpp"
#include "EpollManager.hpp"
#include "Logger.hpp"
#include "ServerManager.hpp"
#include "ConfigFile.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"


#define QUEUE_CONNEXION 5
#define MAX_EVENTS 100
#define MESSAGE_BUFFER 40000
#define TIMEOUT_CHECK_INTERVAL 10
#define CONNECTION_TIMEOUT 60 
#define BUFFER_SIZE 4096






#endif
