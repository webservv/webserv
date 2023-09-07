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
    clientSockets.insert(std::make_pair(client_sockfd, Router(this, client_addr, configs[socket_fd])));
}

void Server::handlePipeEvent(int identifier, const struct kevent& cur) {
    Router& tmp = *pipes[identifier];
    if (cur.flags & EV_EOF) {
        tmp.readFromCGI();
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
// const std::vector<char>&    response = clientSockets[client_sockfd].getResponse();
// const size_t                size = (response.size() < 500) ? response.size() : 500;
// for (size_t i = 0; i < size; ++i) {
//     std::cout << response[i];
// }
// std::cout << std::endl;
static size_t   num = 0;
std::cout << "Send OK: " << ++num << std::endl;
	close(client_sockfd);
	clientSockets.erase(client_sockfd);
}

// static void timeStamp(int i) {
//     std::time_t Time = std::time(NULL);
//     std::string timeStr = std::ctime(&Time);
//     std::cout << "Time" << i << " : " << timeStr << std::endl;
// }

void Server::receiveBuffer(const int client_sockfd) {
    ssize_t             recvByte;
	std::vector<char>   buf(DEBUG_BUFFER_SIZE);
    Router&             router = clientSockets[client_sockfd];
	if (router.getHaveResponse())
		return;
	recvByte = recv(client_sockfd, buf.data(), DEBUG_BUFFER_SIZE, 0);
	if (recvByte == -1)
		throw std::runtime_error("ERROR on accept. " + std::string(strerror(errno)));
    buf.resize(recvByte);
    router.addRequest(buf);
	if (router.isHeaderEnd()) {
        router.parseRequest();
		if (router.isRequestEnd()) {
			router.handleRequest();
        }
	}
    if (router.isRequestEnd() || router.getHaveResponse()) {
        addIOchanges(client_sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
        addIOchanges(client_sockfd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    }
}

void Server::sendBuffer(const int client_sockfd, const intptr_t bufSize) {
    Router&                     router = clientSockets[client_sockfd];
	const std::vector<char>&    message = router.getResponse();
    const size_t                sentLength = router.getSentLength();
    const size_t                leftLength = message.size() - sentLength;
    size_t                      sendLength;

	if (bufSize < static_cast<intptr_t>(leftLength))
        sendLength = send(client_sockfd, message.data() + sentLength, bufSize, 0);
	else
        sendLength = send(client_sockfd, message.data() + sentLength, leftLength, 0);
    if (sendLength < 0)
		throw std::runtime_error("send error. Server::receiveFromSocket" + std::string(strerror(errno)));
    if (sentLength + sendLength == message.size()) {
        disconnect(client_sockfd);
    }
    else
        router.setSentLength(sendLength + sentLength);
}

void Server::waitEvents() {
    const int events = kevent(kqueueFd, &IOchanges[0], IOchanges.size(), &IOevents[0], IOevents.size(), NULL);
    IOchanges.clear();
std::cout << "events: " << events << std::endl;
    if (events < 0) {
        throw std::runtime_error("kevent error: " + std::string(strerror(errno)));
    }
    for (int i = 0; i < events; ++i) {
        handleEvent(IOevents[i]);
    }
}