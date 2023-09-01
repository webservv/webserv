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
#include <sys/_types/_size_t.h>
#include <sys/_types/_ssize_t.h>
#include <sys/_types/_uintptr_t.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <utility>
#define BUFFER_SIZE 1000 // we should put it 100
#define DEBUG_BUFFER_SIZE 1000000

void Server::handleEvent(const struct kevent& cur) {
    if (cur.flags & EV_ERROR) {
        throw std::runtime_error("waitEvents: " + std::string(strerror(errno)));
    }

    int identifier = static_cast<int>(cur.ident);
    if (listenSockets.find(identifier) != listenSockets.end()) {
        handleSocketEvent(identifier);
    } else if (clientSockets.find(identifier) != clientSockets.end()) {
        handleIOEvent(identifier, cur);
    } else if (pipes.find(identifier) != pipes.end()) {
        handlePipeEvent(identifier, cur);
    }
}

void Server::handleSocketEvent(int socket_fd) {
    sockaddr_in         client_addr;
    socklen_t           client_len = sizeof(client_addr);
    static const size_t MAX_CLIENT_NUM = 3;

    if (clientSockets.size() > MAX_CLIENT_NUM)
        return;
    const int client_sockfd = accept(socket_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
    if (client_sockfd < 0) {
        throw std::runtime_error("ERROR on accept");
    }

    if (fcntl(client_sockfd, F_SETFL, fcntl(client_sockfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
        throw std::runtime_error("fcntl error! " + std::string(strerror(errno)));
    }
    addIOchanges(client_sockfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    addIOchanges(client_sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
    clientSockets.insert(std::make_pair(client_sockfd, Router(this, client_addr, configs[socket_fd])));
}

void Server::handlePipeEvent(int identifier, const struct kevent& cur) {
    Router& tmp = *pipes[identifier];
    if (cur.flags & EV_EOF) {
        tmp.disconnectCGI();
    } else if (cur.filter == EVFILT_READ) {
        tmp.readFromCGI();
    } else if (cur.filter == EVFILT_WRITE) {
        tmp.writeToCGI(cur.data);
    }
}

void Server::handleIOEvent(int identifier, const struct kevent& cur) {
    if (cur.flags & EV_EOF) {
        disconnect(identifier);
    } else if (cur.filter == EVFILT_READ) {
        receiveBuffer(identifier);
    } else if (cur.filter == EVFILT_WRITE && clientSockets[identifier].getHaveResponse()) {
        sendBuffer(identifier, cur.data);
    }
}

void Server::disconnect(const int client_sockfd) {
	close(client_sockfd);
	clientSockets.erase(client_sockfd);
}

void Server::receiveBuffer(const int client_sockfd) {
    ssize_t recvByte;
	char    buf[DEBUG_BUFFER_SIZE + 1] = {0, };
    Router& router = clientSockets[client_sockfd];

	if (router.getHaveResponse())
		return;
	recvByte = recv(client_sockfd, buf, DEBUG_BUFFER_SIZE, 0);
	if (recvByte == -1)
		throw std::runtime_error("ERROR on accept. " + std::string(strerror(errno)));
std::cout << buf << std::endl;
	std::vector<char>   input;
    input.reserve(recvByte);
    for (ssize_t i = 0; i < recvByte; ++i) {
        input.push_back(buf[i]);
    }
    router.addRequest(input);
	if (router.isHeaderEnd()) {
        router.parseRequest();
		if (router.isRequestEnd()) {
const std::vector<char>& request = router.getRequest();
for (size_t i = 0; i < request.size(); ++i) {
std::cout << request[i];
}
std::cout << std::endl;
			router.handleRequest();
        }
	}
}

void Server::sendBuffer(const int client_sockfd, const intptr_t bufSize) {
	const std::string& message = clientSockets[client_sockfd].getResponse();
std::cout << message << std::endl;
	if (bufSize < static_cast<intptr_t>(message.length())) {
		if (send(client_sockfd, message.c_str(), bufSize, 0) < 0)
			throw std::runtime_error("send error. Server::receiveFromSocket" + std::string(strerror(errno)));
		clientSockets[client_sockfd].setResponse(message.substr(bufSize));
	}
	else {
		if (send(client_sockfd, message.c_str(), message.length(), 0) < 0)
			throw std::runtime_error("send error. Server::receiveFromSocket" + std::string(strerror(errno)));
		disconnect(client_sockfd);
		// const sockaddr_in	        tmp = clientSockets[client_sockfd].getClientAddr();
        // const Config::server*       config = clientSockets[client_sockfd].getConfig();
		// clientSockets.erase(client_sockfd);
		// clientSockets.insert(std::make_pair(client_sockfd, Router(this, tmp, config)));
	}
}

void Server::waitEvents() {
    const int events = kevent(kqueueFd, &IOchanges[0], IOchanges.size(), &IOevents[0], IOevents.size(), NULL);
    IOchanges.clear();

    if (events < 0) {
        throw std::runtime_error("kevent error: " + std::string(strerror(errno)));
    }

    for (int i = 0; i < events; ++i) {
        handleEvent(IOevents[i]);
    }
}