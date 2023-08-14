#include "includes.hpp"

int main() {
    const int port = 8080;
    const char* host = "0.0.0.0";

    try {
        Server* server = Server::getInstance();
        server->init(port, host);

        std::cout << "Server started, waiting for connections..." << std::endl;

        while (true) {
            server->acceptConnection();
            server->handlePoll();
        }
        server->stop();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
