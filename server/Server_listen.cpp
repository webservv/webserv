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

static int backlog = 5;
static const std::string    default_host = "127.0.0.1";

int Server::createSocket() {
    int new_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (new_socket_fd < 0) {
        throw std::runtime_error("ERROR opening socket");
    }

    if (fcntl(new_socket_fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC) < 0) {
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


void Server::bindSocket(const Config::server& server, int socket_fd) {
    sockaddr_in server_addr;
    const int port = server.listen_port;
    const std::string& host = default_host;

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

