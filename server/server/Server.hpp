#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>

class Server
{
private:
    static Server* instance;
    
    Server() { }
    Server(const Server&);
    Server& operator=(const Server&);

public:
    ~Server() { }
    static Server* getInstance();
};

Server* Server::instance = NULL;

#endif
