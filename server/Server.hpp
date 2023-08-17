#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/event.h>
#include <istream>
#include <vector>
#include <map>
#define BUFFER_SIZE 1042
#define EVENTS_SIZE 100
class Server
{
private:
	static Server* instance;
private:
	int socket_fd;
	sockaddr_in client_addr;
	int kqueue_fd;
	std::vector<struct kevent> IOchanges;
	std::vector<struct kevent> IOevents;
	std::map<int, std::string> clientMessages;
private:
	Server();
	Server(const int port, const char* host);
	Server(const Server& copy);
	Server& operator=(const Server& copy);
    void receiveBuffer(const int client_sockfd);
    void writeToFile(const char* buf);
    void processRequest(const std::string& buf, const int client_sockfd);
	void addIOchanges(uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata);
	void disconnect(const int client_sockfd);
public:
	~Server();

	// Retrieve the single instance of the Server Class.
	static Server& getInstance(const int port, const char* host);

	// Stop the server and close all connections.
	void stop();

	// Create a socket using the socket() function.
	void createSocket();

	// Optionally set socket options like reusing the address.
	void setSocketOptions();

	// Bind the socket to an address and port number.
	void bindSocket(int port, const char* host);

	// Start listening for client connections using the listen() function.
	void listenSocket();

	// Accept a connection from a client using the accept() function.
	void acceptConnection();

	void waitEvents(void);
};

#endif
