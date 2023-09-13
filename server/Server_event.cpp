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
#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <utility>
#include <vector>
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
    static const size_t MAX_CLIENT_NUM = 1000;
    struct linger       opt = {1, 0};

    if (clientSockets.size() > MAX_CLIENT_NUM) {
        return;
    }
    const int client_sockfd = accept(socket_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
    if (client_sockfd < 0) {
        throw std::runtime_error("ERROR on accept");
    }
    if (setsockopt(socket_fd, SOL_SOCKET, SO_LINGER, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("fcntl error! " + std::string(strerror(errno)));
    if (fcntl(client_sockfd, F_SETFL, O_NONBLOCK, FD_CLOEXEC) < 0) {
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

static void printResponse(const std::vector<char>& response) {
    const size_t    size = response.size() < 500 ? response.size() : 500;

    std::cout << "@@@@@@@@@@@Response@@@@@@@@@@@@@" << std::endl;
    for (size_t i = 0; i < size; ++i) {
        std::cout << response[i];
    }
    std::cout << std::endl;
}

void Server::disconnect(const int client_sockfd) {
static size_t   num = 0;
printResponse(clientSockets[client_sockfd].getResponse());
std::cout << "disconnect: " << ++num << std::endl;
    close(client_sockfd);
    clientSockets.erase(client_sockfd);
}

// #include <sys/time.h>
// static void timeStamp(const std::string& str) {
//     timeval currentTime;
//     gettimeofday(&currentTime, NULL);
//     long milliseconds = currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
//     std::cout << str << ": " << milliseconds << std::endl;
// }

static void printRequest(const std::vector<char>& request) {
    const size_t    size = request.size() < 500 ? request.size() : 500;

    std::cout << "#############Request###########" << std::endl;
    for (size_t i = 0; i < size; ++i) {
        std::cout << request[i];
    }
    std::cout << std::endl;
}

void Server::receiveBuffer(const int client_sockfd) {
    ssize_t             recvByte;
    Buffer              buf;
    Router&             router = clientSockets[client_sockfd];

	if (router.getHaveResponse())
		return;
	recvByte = recv(client_sockfd, buf.begin(), buf.getSafeSize(DEBUG_BUFFER_SIZE), 0);
	if (recvByte == -1)
		throw std::runtime_error("ERROR on accept. " + std::string(strerror(errno)));
    buf.setSize(recvByte);
    router.addRequest(buf);
    if (router.isRequestEnd()) {
        router.handleRequest();
    }
    if (router.isRequestEnd() || router.getHaveResponse()) {
printRequest(router.getRequest());
        addIOchanges(client_sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
        // addIOchanges(client_sockfd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
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
    if (events < 0) {
        throw std::runtime_error("kevent error: " + std::string(strerror(errno)));
    }
    for (int i = 0; i < events; ++i) {
        handleEvent(IOevents[i]);
    }
}
