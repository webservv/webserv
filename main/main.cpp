#include "includes.hpp"

int main() {
    Server* server = Server::getInstance();
    std::cout << server << std::endl;
    return 0;
}