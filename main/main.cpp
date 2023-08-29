
#include "Server.hpp"
#include "Config.hpp"
#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
	static_cast<void>(argc);
	const std::string configPath(argv[1] ? argv[1] : "server.conf");
	try {
        Config config(configPath);
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
