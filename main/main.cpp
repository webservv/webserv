
#include "Server.hpp"
#include "Config.hpp"
#include <cstdlib>

int main(int argc, char** argv) {
	const int port = 8080;
	const char* host = "127.0.0.1";

    if (argc != 2) {
        std::cout << "Usage: ./webserv <config_file>" << std::endl;
        return EXIT_FAILURE;
    }

	try {
        try {
            Config config(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
		Server& server = Server::getInstance(port, host);
		while (true) {
			server.waitEvents();
		}
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}