
#include "Config.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>

Config::Config(const std::string& config_file) 
{
	std::fstream configParser;
	configParser.open(config_file);

	std::string line;
	while (std::getline(configParser, line)) {
		if (line.front() == '#' || line.length() == 0)
			continue;
		if (line.back() != ';' && line.back() != '{' && line.back() != '}') {
			std::cout << line << std::endl;
			throw std::out_of_range("invalid config - line");
		}
		tokenization(line);
	}

	while (!tokens.empty()) {
		if (tokens.front() == "http") {
			tokens.pop();
			if (tokens.front() == "{") {
				tokens.pop();
				parseHTTP();
			}
		}
		else
			throw std::out_of_range("invalid config - constructor");
	}
}

void Config::tokenization(const std::string& line) {

	std::istringstream tokenStream(line);
	std::string token;
	while (std::getline(tokenStream, token, ' ')) {
		if (!token.empty()) {
			tokens.push(token);
			// std::cout << "Token: " << token << std::endl;
		}
	}
}

void Config::parseHTTP(void) {
	while (tokens.front() != "}") {
		if (tokens.front() == "server") {
			tokens.pop();
			if (tokens.front() == "{") {
				tokens.pop();
				parseServer();
			}
		}
		else
			throw std::out_of_range("invalid config - http");
	}
	tokens.pop();
}

void Config::parseServer(void) {

	server new_server;
	new_server.clinetMaxSize = 1;
	std::string buf;

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
			tokens.pop();
			parseLocation(new_server);
		}
		else if (tokens.front() == "error_page") {
			tokens.pop();
			parseErrorPage(new_server);
		}
		else if (tokens.front() == "client_max_body_size") {
			tokens.pop();
			std::string size = tokens.front();
			tokens.pop();
			size.pop_back();
			if (size.back() == 'M') {
				size.pop_back();
				new_server.clinetMaxSize = std::stoi(size);
			}
			else
				throw std::out_of_range("invalid config - max size");
		}
		else
			throw std::out_of_range("invalid config - server");
	}
	servers.push_back(new_server);
	tokens.pop();
}

void Config::parseLocation(server& server) {
	
	location new_location;
	
	new_location.url = tokens.front();
	tokens.pop();

	if (tokens.front() == "{") {
		tokens.pop();
	}
	std::string buf;

	while (tokens.front() != "}") {
		std::string key = tokens.front();
		if (key == "root" || key == "index" || key == "return" || key == "upload_store") {
			tokens.pop();
			std::string value = tokens.front();
			value.pop_back();
			new_location.info[key] = value;
			tokens.pop();
		}
		else if (tokens.front() == "limit_except") {
			tokens.pop();
			while (tokens.front().back() != ';') {
				new_location.allowedMethod.push_back(tokens.front());
				tokens.pop();
			}
			std::string method = tokens.front();
			method.pop_back();
			new_location.allowedMethod.push_back(method);
			tokens.pop();
		}
		else if (tokens.front() == "autoindex") {
			tokens.pop();
			if (tokens.front() == "on;")
				new_location.autoIndex = true;
			else if (tokens.front() == "off;")
				new_location.autoIndex = false;
			else
				throw std::out_of_range("invalid config - autoindex");
			tokens.pop();
		}
		else
			throw std::out_of_range("invalid config - location");
	}
	tokens.pop();
	server.locations.push_back(new_location);
}

void Config::parseErrorPage(server& server) {
	
	int errorCode = std::stoi(tokens.front());
	tokens.pop();
	std::string errorPage = tokens.front();
	errorPage.pop_back();
	tokens.pop();
	server.errorPages.insert(std::pair<int, std::string>(errorCode, errorPage));
}

Config::~Config() {}
Config::Config(const Config& copy) {
}
Config& Config::operator=(const Config& copy) {
	static_cast<void>(copy);
	return *this;
}
