
#include "Server.hpp"

int main() {
	const int port = 8080;
	const char* host = "127.0.0.1";

	try {
		Server& server = Server::getInstance(port, host);
		std::cout << "Server started, waiting for connections..." << std::endl;

		while (true) {
			server.waitEvents();
		}
		server.stop();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
