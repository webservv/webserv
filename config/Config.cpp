
#include "Config.hpp"
#include <iostream>
#include <fstream>

Config::Config(const std::string& config_file) 
{
	std::fstream configParser;
	configParser.open(config_file);
	// static int depth = 0;

	std::string line;
	while (std::getline(configParser, line)) {
		if (line.front() == '#' || line.length() == 0)
			continue;
		if (line.back() != ';' && line.back() != '{' && line.back() != '}')
			throw std::out_of_range("invalid config");
		tokenization(line);
	}

	while (!tokens.empty()) {
		if (tokens.front() == "server") {
			tokens.pop();
			if (tokens.front() == "{") {
				tokens.pop();
				parseServer();
			}
		}
		// other than server ? 
		// else -> error
	}
}

void Config::tokenization(const std::string& line) {

	std::istringstream tokenStream(line);
	std::string token;
	while (std::getline(tokenStream, token, ' ')) {
		if (!token.empty()) {
			tokens.push(token);
		}
	}
}

void Config::parseServer(void) {

	server new_server;
	std::string buf;

	std::cout << "starting parse server" << std::endl;
	while (tokens.front() != "}") {
		if (tokens.front() == "listen") {
			tokens.pop();
			buf = tokens.front();
			buf.pop_back();
			new_server.listen_port = std::stoi(buf);
			tokens.pop();
		}
		else if (tokens.front() == "server_name") {
			tokens.pop();
			buf = tokens.front();
			buf.pop_back();
			new_server.server_name = buf;
			tokens.pop();
		}
		else if (tokens.front() == "location") {
			parseLocation(new_server);
		}
		// else 
			// somthing else ? 
	}
	servers.push_back(new_server);
	tokens.pop();
}

void Config::parseLocation(server& server) {
	
	std::cout << "starting parse location" << std::endl;
	location new_location;
	
	tokens.pop();
	new_location.url = tokens.front();
	tokens.pop();

	if (tokens.front() == "{") {
		tokens.pop();
	}
	std::string buf;

	while (tokens.front() != "}") {
		if (tokens.front() == "root") {
			tokens.pop();
			buf = tokens.front();
			buf.pop_back();
			new_location.root = buf;
			tokens.pop();
		}
		else if (tokens.front() == "index") {
			tokens.pop();
			buf = tokens.front();
			buf.pop_back();
			new_location.index = buf;
			tokens.pop();
		}
		// else 
			// somthing else ? 
	}
	tokens.pop();
	std::cout << "location" << std::endl 
		<< "url  : " << new_location.url << std::endl
		<< "root : " << new_location.root << std::endl
		<< "index: " << new_location.index << std::endl;
	server.locations.push_back(new_location);
}

Config::~Config() {}
Config::Config(const Config& copy) {
}
Config& Config::operator=(const Config& copy) {
	static_cast<void>(copy);
	return *this;
}
