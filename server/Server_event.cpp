#include "Server.hpp"

void Server::handleEvent(const struct kevent& cur) {
    if (cur.flags & EV_ERROR) {
        throw std::runtime_error("waitEvents: " + std::string(strerror(errno)));
    }

    const int identifier = static_cast<int>(cur.ident);
    if (listenSockets.find(identifier) != listenSockets.end()) {
        handleSocketEvent(identifier);
    } else if (clientSockets.find(identifier) != clientSockets.end()) {
        handleIOEvent(identifier, cur);
    } else if (pipes.find(identifier) != pipes.end()) {
        handlePipeEvent(identifier, cur);
    }
}

void Server::handlePipeEvent(int identifier, const struct kevent& cur) {
    Router& tmp = *pipes[identifier];
    if (cur.flags & EV_EOF) {
        tmp.readFromCGI();
        tmp.disconnectCGI();
    } else if (cur.filter == EVFILT_READ) {
        tmp.readFromCGI();
    } else if (cur.filter == EVFILT_WRITE) {
        tmp.writeToCGI(cur.data);
    }
}

void Server::waitEvents() {
    const int events = kevent(kqueueFd, &IOchanges[0], IOchanges.size(), &IOevents[0], IOevents.size(), NULL);
    IOchanges.clear();
std::cout << "events: " << events << std::endl;
    if (events < 0) {
        throw std::runtime_error("kevent error: " + std::string(strerror(errno)));
    }
    for (int i = 0; i < events; ++i) {
        handleEvent(IOevents[i]);
    }
}
