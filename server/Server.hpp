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

class Server
{
private:
	static Server* instance;
private:
    int                                     kqueueFd;
    std::map<int, int>                      listenSockets;
    std::vector<struct kevent>              IOchanges;
    std::vector<struct kevent>              IOevents;
    std::map<int, Router>                   clientSockets;
    std::map<int, Router*>                  pipes;
    std::map<std::string, std::string>      cookies;
    std::map<int, const Config::server*>    configs;
//Server_event.cpp
private:
    void        handleEvent(const struct kevent& cur);
    void        handleSocketEvent(int socket_fd);
    void        handlePipeEvent(int identifier, const struct kevent& cur);
    void        handleIOEvent(int identifier, const struct kevent& cur);
	void        disconnect(const int client_sockfd);
    void        receiveBuffer(const int client_sockfd);
	void        sendBuffer(const int client_sockfd, const intptr_t bufSize);
public:
	void        waitEvents(void);
//Server_listen.cpp
private:
	int         createSocket();
	void        setSocketOptions(int socket_fd) const;
	void        bindSocket(const Config::server& server, int socket_fd) const;
	void        listenSocket(int socket_fd) const;
//Server.cpp
private: 
	Server();
	Server(const Config& config);
	Server(const Server& copy);
	Server& operator=(const Server& copy);
public:
	~Server();
private:
	void        addIOchanges(uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata);
	in_addr_t   IPToInt(const std::string& ip) const;
    int         getRequestError(const int clientSocketFD) const;
public:
	static Server&      getInstance(const Config& config);
	void                addPipes(const int writeFd, const int readFd, Router* const router);
    void                addCookie(const std::string& key, const std::string& value);
    const std::string&  getCookie(const std::string& key) const;
};

#endif