#include "Server.hpp"

static void bufferToRouter(Router& router, std::vector<char>& buf);
static bool isEOFEvent(const struct kevent& cur);
static bool isReadEvent(const struct kevent& cur);
static bool isWriteEvent(const struct kevent& cur, Router& router);
static bool isBufferSmallerThanLeft(const intptr_t& bufSize, const size_t& leftLength);
static bool isSendError(const ssize_t& sendLength);
static bool isMessageCompletelySent(const size_t& sentLength, const size_t& sendLength, \
            const size_t& messageSize);
static void giveCurrentLengthToRouter(Router& router, size_t length);
static bool isRouterHaveResponse(const Router& router);

void Server::handleIOEvent(int identifier, const struct kevent& cur) {
    if (isEOFEvent(cur)) {
        disconnect(identifier);
    } else if (isReadEvent(cur)) {
        receiveBuffer(identifier);
    } else if (isWriteEvent(cur, clientSockets[identifier])) {
        sendBuffer(identifier, cur.data);
    }
}


void Server::receiveBuffer(const int client_sockfd) {
    const int           buffer_size = 1000000;
    ssize_t             recvByte;
	std::vector<char>   buf(buffer_size);
    Router&             router = clientSockets[client_sockfd];

    if (isRouterHaveResponse(router)) {
        return;
    }
	recvByte = recv(client_sockfd, buf.data(), buffer_size, 0);
	if (recvByte == -1)
		throw std::runtime_error("ERROR on accept. " + std::string(strerror(errno)));
    buf.resize(recvByte);
    bufferToRouter(router, buf);
    if (router.getHaveResponse()) {
        AddIOReadDelete(client_sockfd);
        AddIOWriteChange(client_sockfd);
    }
}

void Server::AddIOReadDelete(uintptr_t ident) {
    struct kevent newEvents;
    EV_SET(&newEvents, ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    IOchanges.push_back(newEvents);
}

void Server::sendBuffer(const int client_sockfd, const intptr_t bufSize) {
    Router& router = clientSockets[client_sockfd];
    const std::vector<char>& message = router.getResponse();
    const size_t sentLength = router.getSentLength();
    const size_t leftLength = message.size() - sentLength;
    size_t sendLength;

    sendLength = getSendLength(client_sockfd, sentLength, bufSize, leftLength, message.data());
    if (isSendError(sendLength))
        throw std::runtime_error("send error. Server::receiveFromSocket" + std::string(strerror(errno)));
    if (isMessageCompletelySent(sentLength, sendLength, message.size())) {
        disconnect(client_sockfd);
    } else {
        giveCurrentLengthToRouter(router, sendLength + sentLength);
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

void Server::AddIOReadChange(uintptr_t ident) {
    struct kevent newEvents;
    EV_SET(&newEvents, ident, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    IOchanges.push_back(newEvents);
}

void Server::AddIOWriteChange(uintptr_t ident) {
    struct kevent newEvents;
    EV_SET(&newEvents, ident, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
    IOchanges.push_back(newEvents);
}

static void bufferToRouter(Router& router, std::vector<char>& buf) {
    router.addRequest(buf);
	if (router.isHeaderEnd()) {
        router.parseRequest();
		if (router.isRequestEnd()) {
			router.handleRequest();
        }
	}
}

static bool isEOFEvent(const struct kevent& cur) {
    return cur.flags & EV_EOF;
}

static bool isReadEvent(const struct kevent& cur) {
    return cur.filter == EVFILT_READ;
}

static bool isWriteEvent(const struct kevent& cur, Router& router) {
    return cur.filter == EVFILT_WRITE && router.getHaveResponse();
}

static bool isBufferSmallerThanLeft(const intptr_t& bufSize, const size_t& leftLength) {
    return bufSize < static_cast<intptr_t>(leftLength);
}

static bool isSendError(const ssize_t& sendLength) {
    return sendLength < 0;
}

static bool isMessageCompletelySent(const size_t& sentLength, const size_t& sendLength, \
    const size_t& messageSize) {
    return sentLength + sendLength == messageSize;
}

size_t Server::getSendLength(const int client_sockfd, \
    const size_t sentLength, const intptr_t bufSize, const size_t leftLength, \
    const void* data) {
    const char* char_data = static_cast<const char*>(data);
    if (isBufferSmallerThanLeft(bufSize, leftLength))
        return send(client_sockfd, char_data + sentLength, bufSize, 0);
    else
        return send(client_sockfd, char_data + sentLength, leftLength, 0);
}


static void giveCurrentLengthToRouter(Router& router, size_t length) {
    router.setSentLength(length);
}

static bool isRouterHaveResponse(const Router& router) {
    return router.getHaveResponse();
}

