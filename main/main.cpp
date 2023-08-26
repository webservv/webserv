
#include "Server.hpp"
#include "Config.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: ./webserv <config_file>" << std::endl;
        return 1;
    }

	try {
        Config config(argv[1]);
        Server& server = Server::getInstance(config);
		while (true) {
			server.waitEvents();
		}
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
