#include "Server.hpp"
#include "Request.hpp"
#include "Router.hpp"
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <sys/_types/_int16_t.h>
#include <sys/_types/_intptr_t.h>
#include <sys/_types/_uintptr_t.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <cstdio>

static int backlog = 5;
static const std::string    post_txt = "./document/posts.txt";

Server* Server::instance = NULL;

Server::Server() : socket_fd(-1) {}

Server::Server(const int port, const char* host): socket_fd(-1), IOevents(EVENTS_SIZE) {
	kqueue_fd = kqueue();
	if (kqueue_fd < 0)
		throw std::runtime_error("kqueue error. " + std::string(strerror(errno)));
	createSocket();
	setSocketOptions();
	bindSocket(port, host);
	listenSocket();
    std::remove(post_txt.c_str());
    std::cout << "Server started, waiting for connections..." << std::endl;
}

Server::Server(const Server& copy) { static_cast<void>(copy); }

Server& Server::operator=(const Server& copy) {
	static_cast<void>(copy);
	return *this;
}

Server::~Server() {
	stop();
}

Server& Server::getInstance(const int port, const char* host) {
	if (instance == NULL) {
		instance = new Server(port, host);
	}
	return *instance;
}

void Server::stop() {
	if (socket_fd != -1) {
		close(socket_fd);
		socket_fd = -1;
	}
	for (std::map<int, std::string>::iterator it = clientMessages.begin(); it != clientMessages.end(); it++) {
		disconnect(it->first);
	}
	close(kqueue_fd);
}

void Server::createSocket() {
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		throw std::runtime_error("ERROR opening socket");
	}
	if (fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL, 0) | O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl error! " + std::string(strerror(errno)));
	addIOchanges(socket_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
}

void Server::setSocketOptions() {
	int opt = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		throw std::runtime_error("ERROR setting socket options");
	}
}

void Server::bindSocket(int port, const char* host) {
	sockaddr_in	server_addr;

	std::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(host);
	if (server_addr.sin_addr.s_addr == INADDR_NONE) {
		throw std::runtime_error("ERROR invalid host");
	}
	if (bind(socket_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
		throw std::runtime_error("ERROR on binding");
	}
}

void Server::listenSocket() {
	if (listen(socket_fd, backlog) < 0) {
		throw std::runtime_error("ERROR on listening");
	}
}

void Server::acceptConnection() {
	socklen_t client_len = sizeof(client_addr);
	const int client_sockfd = accept(socket_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);

	if (client_sockfd < 0) {
		throw std::runtime_error("ERROR on accept");
	}
	if (fcntl(client_sockfd, F_SETFL, fcntl(client_sockfd, F_GETFL, 0) | O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl error! " + std::string(strerror(errno)));
	addIOchanges(client_sockfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	addIOchanges(client_sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
	clientMessages[client_sockfd] = "";
}

void Server::receiveBuffer(const int client_sockfd) {
    int recvByte;
	char buf[BUFFER_SIZE] = {0, };

    while (true) {
        recvByte = recv(client_sockfd, buf, BUFFER_SIZE, 0);
		clientMessages[client_sockfd] += buf;
        if (recvByte == -1)
            throw std::runtime_error("ERROR on accept. " + std::string(strerror(errno)));
        if (recvByte < BUFFER_SIZE)
            break;
    }
	writeToFile(buf);
}

void Server::writeToFile(const char* buf) {
	std::ofstream out_file;
	out_file.open("request");
	out_file << buf;
	out_file.close();
}

void Server::processRequest(const std::string& buf, const int client_sockfd) {
    try {
        Router router(buf, client_sockfd);
        router.handleRequest();
        if (send(client_sockfd, router.getResponseStr().c_str(), router.getResponseStr().length(), 0) < 0)
            throw std::runtime_error("send error. Server::receiveFromSocket" + std::string(strerror(errno)));
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

void Server::addIOchanges(uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata) {
	struct kevent	newEvents;

	EV_SET(&newEvents, ident, filter, flags, fflags, data, udata);
	IOchanges.push_back(newEvents);
}

void Server::waitEvents(void) {
	const int events = kevent(kqueue_fd, &IOchanges[0], IOchanges.size(), &IOevents[0], IOevents.size(), NULL);
	
	IOchanges.clear();
	if (events < 0)
		throw std::runtime_error("kevent error! " + std::string(strerror(errno)));
	for (int i = 0; i < events; i++) {
		const struct kevent& cur = IOevents[i];
		if (cur.flags & EV_ERROR)
			throw std::runtime_error("kevent EV_ERROR!");
		else if (static_cast<int>(cur.ident) == socket_fd)
			acceptConnection();
		else if (clientMessages.find(cur.ident) != clientMessages.end()) {
			if (cur.filter == EVFILT_READ) {
				if (cur.flags & EV_EOF)
					disconnect(cur.ident);
				else
					receiveBuffer(cur.ident);
			}
			else if (cur.filter == EVFILT_WRITE && clientMessages[cur.ident] != "") {
				processRequest(clientMessages[cur.ident], cur.ident);
				clientMessages[cur.ident].clear();
			}
		}
	}
}

void Server::disconnect(const int client_sockfd) {
	close(client_sockfd);
	clientMessages.erase(client_sockfd);
}