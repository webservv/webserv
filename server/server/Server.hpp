#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

class Server
{
private:
    static Server* instance;
    int socket_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    struct pollfd poll_fd[256];
    int num_connections;
    
    Server();
    Server(const Server& copy);
    Server& operator=(const Server& copy);

public:
    ~Server();

    // Retrieve the single instance of the Server Class.
    static Server* getInstance();

    // Configure the server with settings such as port and host.
    void init(int port, const char* host);

    // Manage all connections using poll() (or equivalent).
    void handlePoll();

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
};

#endif
