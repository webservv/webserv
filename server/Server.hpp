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
#include "Router.hpp"
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
	std::map<int, Router> routers;
private:
	Server();
	Server(const int port, const char* host);
	Server(const Server& copy);
	Server& operator=(const Server& copy);
public:
	~Server();
private:
    void receiveBuffer(const int client_sockfd);
    void writeToFile(const char* buf);
    void processRequest(const std::string& buf, const int client_sockfd);
	void addIOchanges(uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata);
	void disconnect(const int client_sockfd);
	void sendBuffer(const int client_sockfd, const int64_t bufSize);
public:
	static Server& getInstance(const int port, const char* host);
	void createSocket();
	void setSocketOptions();
	void bindSocket(int port, const char* host);
	void listenSocket();
	void acceptConnection();
	void waitEvents(void);
};

#endif