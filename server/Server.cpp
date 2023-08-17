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
#include <unistd.h>

static int backlog = 5;

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
}

Server::Server(const Server& copy) { static_cast<void>(copy); }

Server& Server::operator=(const Server& copy) {
	static_cast<void>(copy);
	return *this;
}

Server::~Server() {
	stop();
}

// We use a singleton pattern here to ensure that only one instance of the Server class is created.
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

/*
	 int
	 socket(int domain, int type, int protocol);
	 domain : AF_INET, AF_INET6, AF_UNIX
		- AF_INET : IPv4 Internet protocols
		- AF_INET6 : IPv6 Internet protocols
		- AF_UNIX : Local communication protocols
	 type : SOCK_STREAM, SOCK_DGRAM, SOCK_RAW
		- SOCK_STREAM : Provides sequenced, reliable, two-way, connection-based byte streams.
		- SOCK_DGRAM : Supports datagrams (connectionless, unreliable messages of a fixed maximum length).
		- SOCK_RAW : Provides raw network protocol access.
	 protocol : 0
*/
void Server::createSocket() {
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		throw std::runtime_error("ERROR opening socket");
	}
	addIOchanges(socket_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
}


/* 
	 int
	 setsockopt(int socket, int level, int option_name,
		 const void *option_value, socklen_t option_len);

		socket : socket file descriptor
		level : SOL_SOCKET, IPPROTO_TCP, IPPROTO_IP
			- SOL_SOCKET : Socket-level option
			- IPPROTO_TCP : TCP-level option
			- IPPROTO_IP : IP-level option
		option_name : SO_REUSEADDR
			- SO_REUSEADDR : Reuse of local addresses
		option_value : 1
		option_len : sizeof(option_value)
*/
void Server::setSocketOptions() {
	int opt = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		throw std::runtime_error("ERROR setting socket options");
	}
}

/*
socketaddr_in :
	short sin_family; // e.g. AF_INET, AF_INET6
	unsigned short sin_port; // e.g. htons(3490)
	struct in_addr sin_addr; // see struct in_addr, below
	char sin_zero[8]; // zero this if you want to

htons() : host to network short
	- host byte order to network byte order
	- little endian to big endian
	- 0x1234 -> 0x3412

inet_addr() : IPv4 address string to network byte order

bind() : bind socket to address
	- socket file descriptor
	- sockaddr* addr
	- socklen_t addrlen
*/
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

/*
listen() : listen for connections on a socket
	- socket file descriptor
	- int backlog : maximum length to which the queue of pending connections for sockfd may grow
*/
void Server::listenSocket() {
	if (listen(socket_fd, backlog) < 0) {
		throw std::runtime_error("ERROR on listening");
	}
}

/* 
accept() : accept a connection on a socket
	- socket file descriptor
	- sockaddr* addr
	- socklen_t* addrlen

accept() returns a new file descriptor,
	and all communication on this connection should be done using the new file descriptor.

*/
void Server::acceptConnection() {
	socklen_t client_len = sizeof(client_addr);
	const int client_sockfd = accept(socket_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);

	if (client_sockfd < 0) {
		throw std::runtime_error("ERROR on accept");
	}
	// Handle the new connection here or add it to a list of connections
	addIOchanges(client_sockfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	addIOchanges(client_sockfd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
	clientMessages[client_sockfd] = "";
}

/*
recv() : receive a message from socket 
	- client socket fd
	- address of buffer
	- length 
	- flag 

return value: the number of bytes received
*/

void Server::receiveBuffer(const int client_sockfd) {
    int recvByte;
	char buf[BUFFER_SIZE] = {0, };

    while (true) {
        recvByte = recv(client_sockfd, buf, BUFFER_SIZE, 0);
		clientMessages[client_sockfd] += buf;
		if (recvByte <= 0)
			disconnect(client_sockfd);
        if (recvByte == -1) {
            std::cout << strerror(errno) << std::endl;
            throw std::runtime_error("ERROR on accept");
        }
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
        Router router(buf);
        router.handleRequest();
        if (send(client_sockfd, router.getResponseStr().c_str(), router.getResponseStr().length(), 0) < 0)
            throw std::runtime_error("send error. Server::receiveFromSocket");
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
			if (cur.filter == EVFILT_READ)
				receiveBuffer(cur.ident);
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
