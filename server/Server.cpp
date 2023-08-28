#include "Server.hpp"
#include "Request.hpp"
#include "Router.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/_types/_in_addr_t.h>
#include <sys/_types/_int16_t.h>
#include <sys/_types/_intptr_t.h>
#include <sys/_types/_uintptr_t.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <utility>
#define NULL_FD -1
#define BUFFER_SIZE 100
#define EVENTS_SIZE 100

static const std::string    post_txt = "./document/posts.txt";

Server* Server::instance = NULL;

Server::Server():
	IOchanges(),
	IOevents(EVENTS_SIZE),
	sockets(),
	pipes(),
    cookies() {}

Server::Server(const Config& config)
    : IOchanges(),
      IOevents(EVENTS_SIZE),
      sockets(),
      pipes(),
      cookies()
{
    kqueue_fd = kqueue();
    if (kqueue_fd < 0) {
        throw std::runtime_error("kqueue error: " + std::string(strerror(errno)));
    }

    const std::vector<server>& servers = config.getServers();
    for (std::vector<server>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        const server& new_server = *it;
        
        int new_socket_fd = createSocket();
        setSocketOptions(new_socket_fd);

        bindSocket(new_server, new_socket_fd);
        listenSocket(new_socket_fd);

        struct kevent change;
        EV_SET(&change, new_socket_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
        if (kevent(kqueue_fd, &change, 1, NULL, 0, NULL) == -1) {
            throw std::runtime_error("kevent register error: " + std::string(strerror(errno)));
        }

        socket_fds[new_socket_fd] = new_socket_fd;
        std::cout << "Server started on " << new_server.server_name << ":"
                  << new_server.listen_port << ", waiting for connections..." << std::endl;
    }
}

Server::Server(const Server& copy) { static_cast<void>(copy); }

Server& Server::operator=(const Server& copy) {
	static_cast<void>(copy);
	return *this;
}

Server::~Server() {
    for (std::map<int, Router>::iterator it = sockets.begin(); it != sockets.end(); ++it) {
        disconnect(it->first);
    }
    for (std::map<int, int>::iterator it = socket_fds.begin(); it != socket_fds.end(); ++it) {
        close(it->second);
    }
    close(kqueue_fd);
}

Server& Server::getInstance(const Config& config) {
	if (instance == NULL) {
		instance = new Server(config);
	}
	return *instance;
}

void Server::addIOchanges(uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata) {
	struct kevent	newEvents;

	EV_SET(&newEvents, ident, filter, flags, fflags, data, udata);
	IOchanges.push_back(newEvents);
}

void Server::addPipes(const int writeFd, const int readFd, Router* const router) {
	pipes[writeFd] = router;
	pipes[readFd] = router;
	addIOchanges(writeFd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
	addIOchanges(readFd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
}

int Server::getRequestError(const int client_sockfd) {
    return sockets[client_sockfd].getRequestError();
}

in_addr_t Server::IPToInt(const std::string& ip) const {
	in_addr_t	ret = 0;
	int			tmp = 0;
	int			bitShift = 0;
	
	for (size_t i = 0; i < ip.size(); i++) {
		if (ip[i] == '.') {
			ret += tmp << bitShift;
			tmp = 0;
			bitShift += 8;
			continue;
		}
		tmp = tmp * 10 + ip[i] - '0';
	}
	ret += tmp << bitShift;
	return ret;
}

void Server::addCookie(const std::string& key, const std::string& value) {
    cookies[key] = value;
}

const std::string& Server::getCookie(const std::string& key) {
    return cookies[key];
}