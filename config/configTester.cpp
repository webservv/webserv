
#include "Config.hpp"
#include <iostream>
#include <fstream>

int main(int ac, char** av) {
	if (ac == 1) {
		std::cout << "put argument" << std::endl;
		return 1;
	}
	try {
		Config config(av[1]);
		std::cout << "good parsing!!" << std::endl;
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}
