
#include "Server.hpp"
#include "Config.hpp"
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: ./webserv <config_file>" << std::endl;
        return EXIT_FAILURE;
    }

	try {
        Config config(argv[1]);
        Server& server = Server::getInstance(config);
		while (true) {
			server.waitEvents();
		}
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
