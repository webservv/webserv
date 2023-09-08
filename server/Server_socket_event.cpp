#include "Server.hpp"

static bool isFcntlSetNonBlockSuccessful(int sockfd);
static int GetSocketFlags(int sockfd);
static bool SetSocketNonBlock(int sockfd, int flags);

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
    if (!isFcntlSetNonBlockSuccessful(client_sockfd)) {
        throw std::runtime_error("fcntl error! " + std::string(strerror(errno)));
    }
    AddIOReadChange(client_sockfd);
    clientSockets.insert(std::make_pair(client_sockfd, Router(this, client_addr, configs[socket_fd])));
}


static bool isFcntlSetNonBlockSuccessful(int sockfd) {
    return SetSocketNonBlock(sockfd, GetSocketFlags(sockfd));
}

std::pair<int, Router> Server::CreateSocketRouterPair(int sockfd, Server* server, \
    sockaddr_in& client_addr, const ServerConfig* config) {
    return std::make_pair(sockfd, Router(server, client_addr, config));
}

static int GetSocketFlags(int sockfd) {
    return fcntl(sockfd, F_GETFL, 0);
}

static bool SetSocketNonBlock(int sockfd, int flags) {
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) >= 0;
}
