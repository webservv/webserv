#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <sys/_types/_intptr_t.h>
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
#include "Config.hpp"
#define BUFFER_SIZE 1042
#define EVENTS_SIZE 100

class Server
{
private:
	static Server* instance;
private:
    std::map<int, int> socket_fds;
    int kqueue_fd;
    std::vector<struct kevent> IOchanges;
    std::vector<struct kevent> IOevents;
    std::map<int, Router> sockets;
    std::map<int, Router*> pipes;
    std::map<std::string, std::string> cookies;
    std::vector<server> serverConfigs;
private:
	Server();
	Server(const Config& config);
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
	void sendBuffer(const int client_sockfd, const intptr_t bufSize);
	in_addr_t IPToInt(const std::string& ip) const;
    void handleEvent(const struct kevent& cur);
    void handleSocketEvent(int identifier);
    void handlePipeEvent(int identifier, const struct kevent& cur);
    void handleIOEvent(int identifier, const struct kevent& cur);
public:
	static Server& getInstance(const Config& config);
    const std::vector<server>& getServerConfigs() const;
	int createSocket();
	void setSocketOptions(int socket_fd);
	void bindSocket(const server& server, int socket_fd);
	void listenSocket(int socket_fd);
	void acceptConnection(int socket_fd);
	void addFd(void);
	void waitEvents(void);
	void addPipes(const int writeFd, const int readFd, Router* const router);
    int getRequestError(const int client_sockfd);
    void addCookie(const std::string& key, const std::string& value);
    const std::string& getCookie(const std::string& key);
};

#endif