#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/event.h>
#include <vector>
#include <map>
#include "Router.hpp"
#include "Config.hpp"

class Server
{
private:
	static Server*                          instance;
private:
    int                                     kqueueFd;
    std::map<int, int>                      listenSockets;
    std::vector<struct kevent>              IOchanges;
    std::vector<struct kevent>              IOevents;
    std::map<int, Router>                   clientSockets;
    std::map<int, Router*>                  pipes;
    std::map<std::string, std::string>      cookies;
    std::map<int, const ServerConfig*>      configs;
private: 
	Server();
	Server(const Config& config);
	Server(const Server& copy);
	Server& operator=(const Server& copy);
public:
	~Server();
public:
	static Server&      getInstance(const Config& config);
    void	            waitEvents(void);
	void                addPipes(const int writeFd, const int readFd, Router* const router);
    void                addCookie(const std::string& key, const std::string& value);
    const std::string&  getCookie(const std::string& key) const;
private:
	int 	            createSocket();
std::pair<int, Router>  CreateSocketRouterPair(int sockfd, Server* server, \
                            sockaddr_in& client_addr, const ServerConfig* config);
    void	            handleEvent(const struct kevent& cur);
    void	            handlePipeEvent(int identifier, const struct kevent& cur);
    void	            handleIOEvent(int identifier, const struct kevent& cur);
	void	            disconnect(const int client_sockfd);
    void	            receiveBuffer(const int client_sockfd);
	void	            sendBuffer(const int client_sockfd, const intptr_t bufSize);
    void	            handleSocketEvent(int socket_fd);
    size_t              getSendLength(const int client_sockfd, \
                            const size_t sentLength, const intptr_t bufSize, const size_t leftLength, \
                            const void* data);
    void                AddIOReadChange(uintptr_t ident);
    void                AddIOWriteChange(uintptr_t ident);
    void                AddIOReadDelete(uintptr_t ident);
};

#endif