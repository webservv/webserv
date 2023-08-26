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

static int backlog = 5;
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
      cookies(),
      serverConfigs(config.getServers()) {
    
    const std::vector<server>& servers = config.getServers();
    
    for (std::vector<server>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        const server& new_server = *it;

        int new_socket_fd = createSocket();
        socket_fds.push_back(new_socket_fd);
        setSocketOptions(new_socket_fd);
        socket_fds.push_back(new_socket_fd);
    
        bindSocket(new_server, new_socket_fd);
        listenSocket(new_socket_fd);
    
        int new_kqueue_fd = kqueue();
        if (new_kqueue_fd < 0) {
            throw std::runtime_error("kqueue error. " + std::string(strerror(errno)));
        }
        kqueue_fds.push_back(new_kqueue_fd);
        std::cout << "Server started on " << new_server.server_name << ":" \
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
    for (std::vector<int>::iterator it = socket_fds.begin(); it != socket_fds.end(); ++it) {
        close(*it);
    }
    for (std::vector<int>::iterator it = kqueue_fds.begin(); it != kqueue_fds.end(); ++it) {
        close(*it);
    }
}

Server& Server::getInstance(const Config& config) {
	if (instance == NULL) {
		instance = new Server(config);
	}
	return *instance;
}

int Server::createSocket() {
    int new_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (new_socket_fd < 0) {
        throw std::runtime_error("ERROR opening socket");
    }

    if (fcntl(new_socket_fd, F_SETFL, fcntl(new_socket_fd, F_GETFL, 0) | O_NONBLOCK) < 0) {
        throw std::runtime_error("fcntl error! " + std::string(strerror(errno)));
    }

    addIOchanges(new_socket_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    return new_socket_fd;
}

void Server::setSocketOptions(int socket_fd) {
    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("ERROR setting socket options");
    }
}

void Server::bindSocket(const server& server, int socket_fd) {
    sockaddr_in server_addr;
    const int port = server.listen_port;
    const std::string& host = server.server_name;

std::cout << "Binding to address: " << host << " Port: " << port << std::endl;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = IPToInt(host);
    
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        throw std::runtime_error("ERROR invalid host");
    }

    if (bind(socket_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        throw std::runtime_error("ERROR on binding: " + std::string(strerror(errno)));
    }
}



void Server::listenSocket(int socket_fd)  {
	if (listen(socket_fd, backlog) < 0) {
		throw std::runtime_error("ERROR on listening");
	}
}

void Server::acceptConnection(int socket_fd) {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    const int client_sockfd = accept(socket_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);

    if (client_sockfd < 0) {
        throw std::runtime_error("ERROR on accept");
    }

    if (fcntl(client_sockfd, F_SETFL, fcntl(client_sockfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
        throw std::runtime_error("fcntl error! " + std::string(strerror(errno)));
    }

    addIOchanges(client_sockfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    addIOchanges(client_sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
    sockets.insert(std::make_pair(client_sockfd, Router(this, client_addr)));
}

const std::vector<server>& Server::getServerConfigs() const {
    return serverConfigs;
}

void Server::addFd(void) {

}

void Server::receiveBuffer(const int client_sockfd) {
    int recvByte;
	char buf[BUFFER_SIZE] = {0, };

	if (sockets[client_sockfd].getHaveResponse())
		return;
	recvByte = recv(client_sockfd, buf, BUFFER_SIZE, 0);
	if (recvByte == -1)
		throw std::runtime_error("ERROR on accept. " + std::string(strerror(errno)));
	sockets[client_sockfd].addRequest(std::string(buf));
	if (sockets[client_sockfd].isHeaderEnd()) {
        try {
		    sockets[client_sockfd].parse();
        } catch (const std::exception& e) {
            int error = getRequestError(client_sockfd);
            sockets[client_sockfd].makeErrorResponse(error);
            return;
        }
		if (sockets[client_sockfd].isRequestEnd())
			sockets[client_sockfd].handleRequest();
	}
}

void Server::addIOchanges(uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata) {
	struct kevent	newEvents;

	EV_SET(&newEvents, ident, filter, flags, fflags, data, udata);
	IOchanges.push_back(newEvents);
}

void Server::waitEvents(void) {
    for (std::vector<int>::const_iterator kq_it = kqueue_fds.begin(); kq_it != kqueue_fds.end(); ++kq_it) {
        int current_kqueue_fd = *kq_it;
        
        const int events = kevent(current_kqueue_fd, &IOchanges[0], IOchanges.size(), &IOevents[0], IOevents.size(), NULL);
        IOchanges.clear();
        
        if (events < 0) {
            throw std::runtime_error("kevent error! " + std::string(strerror(errno)));
        }
        
        for (int i = 0; i < events; ++i) {
            handleEvent(IOevents[i]);
        }
    }
}

void Server::handleEvent(const struct kevent& cur) {
    if (cur.flags & EV_ERROR) {
        throw std::runtime_error("waitEvents: " + std::string(strerror(errno)));
    }

    int identifier = static_cast<int>(cur.ident);
    if (std::find(socket_fds.begin(), socket_fds.end(), identifier) != socket_fds.end()) {
        handleSocketEvent(identifier);
    } else if (sockets.find(identifier) != sockets.end()) {
        handleIOEvent(identifier, cur);
    } else if (pipes.find(identifier) != pipes.end()) {
        handlePipeEvent(identifier, cur);
    }
}

void Server::handleSocketEvent(int identifier) {
    acceptConnection(identifier);
}

void Server::handlePipeEvent(int identifier, const struct kevent& cur) {
    Router& tmp = *pipes[identifier];
    if (cur.flags & EV_EOF) {
        tmp.disconnectCGI();
    } else if (cur.filter == EVFILT_READ) {
        tmp.readCGI();
    } else if (cur.filter == EVFILT_WRITE) {
        tmp.writeCGI(cur.data);
    }
}

void Server::handleIOEvent(int identifier, const struct kevent& cur) {
    if (cur.flags & EV_EOF) {
        disconnect(identifier);
    } else if (cur.filter == EVFILT_READ) {
        receiveBuffer(identifier);
    } else if (cur.filter == EVFILT_WRITE && sockets[identifier].getHaveResponse()) {
        sendBuffer(identifier, cur.data);
    }
}

void Server::disconnect(const int client_sockfd) {
	close(client_sockfd);
	sockets.erase(client_sockfd);
}

void Server::sendBuffer(const int client_sockfd, const intptr_t bufSize) {
	const std::string& message = sockets[client_sockfd].getResponse();
// std::cout << message << std::endl;
	if (bufSize < static_cast<intptr_t>(message.length())) {
		if (send(client_sockfd, message.c_str(), bufSize, 0) < 0)
			throw std::runtime_error("send error. Server::receiveFromSocket" + std::string(strerror(errno)));
		sockets[client_sockfd].setResponse(message.substr(bufSize));
	}
	else {
		if (send(client_sockfd, message.c_str(), message.length(), 0) < 0)
			throw std::runtime_error("send error. Server::receiveFromSocket" + std::string(strerror(errno)));
		
		const sockaddr_in	tmp = sockets[client_sockfd].getClientAddr();
		sockets.erase(client_sockfd);
		sockets.insert(std::make_pair(client_sockfd, Router(this, tmp)));
	}
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