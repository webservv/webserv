#include "Server.hpp"
#include "Request.hpp"

#include <fstream>

static int backlog = 5;

Server* Server::instance = NULL;

Server::Server() : socket_fd(-1), num_connections(0) {}

Server::Server(const int port, const char* host): socket_fd(-1), num_connections(0) {
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

/*
	int poll(struct pollfd *fds, nfds_t nfds, int timeout);
		fds : array of pollfd structs
		nfds : number of fds
		timeout : timeout in milliseconds
	struct pollfd {
		int fd; // file descriptor
		short events; // requested events
		short revents; // returned events
	};
	POLLIN : There is data to read.
	POLLOUT : Writing now will not block.
	POLLERR : Error condition.
*/

void Server::handlePoll() {
	int ret = poll(poll_fd, num_connections, -1);
	if (ret < 0) {
		throw std::runtime_error("ERROR on poll");
	}
	//
}


void Server::stop() {
	if (socket_fd != -1) {
		close(socket_fd);
		socket_fd = -1;
	}
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
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(host);
	if (server_addr.sin_addr.s_addr == INADDR_NONE) {
		throw std::runtime_error("ERROR invalid host");
	}
	if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
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
	
	client_sockfd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_sockfd < 0) {
		throw std::runtime_error("ERROR on accept");
	}
	// Handle the new connection here or add it to a list of connections
}

/*
recv() : receive a message from socket 
	- client socket fd
	- address of buffer
	- length 
	- flag 

return value: the number of bytes received
*/
void Server::receiveFromSocket() {
	char buf[BUFFER_SIZE] = {0, };				// NULL init
	int	recvByte;

	while (true) {
		memset(buf, 0, sizeof(buf));
		recvByte = recv(client_sockfd, buf, sizeof(buf), 0);

		if (recvByte == -1) {
			std::cout << strerror(errno) << std::endl;
			std::cout << errno << std::endl;
			throw std::runtime_error("ERROR on accept");
		}

		// std::cout << "Received: " << buf << std::endl;
		
		std::string string(buf);
		send(client_sockfd, buf, string.length(), 0);

		// std::cout << "Sent: " << buf << std::endl;
		if (recvByte < BUFFER_SIZE)
			break;
	}

	std::ofstream	out_file;
	
	out_file.open("request");
	out_file << buf;
	out_file.close();

	try {
		Request request(buf);
		// also handle request
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}
	/*
	for now, this server is working like echo server. send the exact same message they receive. 
	we will replace this part with server's actions (HTTP...)
	*/

}