#include "Server.hpp"

Server* Server::getInstance() {
    if (instance == NULL) {
        instance = new Server();
    }
    return instance;
}
