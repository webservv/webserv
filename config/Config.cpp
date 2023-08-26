
#include "Config.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>

Config::Config(const std::string& config_file) 
{
	std::fstream configParser;
	configParser.open(config_file);
    if (!configParser.is_open()) {
        throw std::runtime_error("config file open error");
    }
    parseLine(configParser);
	while (!tokens.empty()) {
		std::string token = tokens.front();
		tokens.pop();

		if (token == "http") {
			if (tokens.front() == "{") {
				tokens.pop();
				parseHTTP();
			} else {
                throw std::out_of_range("missing '{' after http");
            }
		} else {
			throw std::out_of_range("invalid config - constructor");
		}
	}
}

void Config::parseLine(std::fstream& configParser) {
	std::string line;
	while (std::getline(configParser, line)) {
		size_t commentPos = line.find('#');
		if (commentPos != std::string::npos) {
			line = line.substr(0, commentPos);
		}
		trim(line);
		if (line.empty())
			continue;
		if (line.back() != ';' && line.back() != '{' && line.back() != '}') {
			throw std::out_of_range("invalid config - line");
		}
		tokenization(line);
    }
}

void Config::trim(std::string &str) const {
    std::string::iterator start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        ++start;
    }

    if (start == str.end()) {
        str.clear();
        return;
    }

    std::string::iterator end = str.end();
    --end;
    while (end != start && std::isspace(*end)) {
        --end;
    }

    str = std::string(start, end + 1);
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


void Config::parseHTTP(void) {
	while (tokens.front() != "}") {
        std::string title = tokens.front();
        tokens.pop();
		if (title == "server")
			parseServer();
		else if (title == "client_max_body_size")
            parseClientMaxBodySize();
		else {
			throw std::out_of_range("invalid config - http");
		}
	}
	tokens.pop();
}

void Config::parseClientMaxBodySize() {
    std::string valueStr = tokens.front();
    tokens.pop();

    if (valueStr.back() != ';') {
        throw std::out_of_range("missing ';' after client_max_body_size");
    }
    valueStr.pop_back();

    int multiplier = 1;
    if (valueStr.back() == 'M') {
        multiplier = 1024 * 1024;
        valueStr.pop_back();
    }

    std::stringstream ss(valueStr);
    ss >> clientMaxBodySize;
    if (ss.fail() || clientMaxBodySize < 0) {
        throw std::out_of_range("invalid client_max_body_size, must be non-negative");
    }

    clientMaxBodySize *= multiplier;
}

void Config::parseServer(void) {

	server new_server;
	std::string buf;

    if (tokens.front() == "{") {
        tokens.pop();
    } else {
        throw std::out_of_range("missing '{' after server");
    }
	while (tokens.front() != "}") {
        std::string title = tokens.front();
        tokens.pop();
		if (title == "listen")
            parseListen(new_server);
		else if (title == "server_name")
            parseServerName(new_server);
		else if (title == "error_page")
			parseErrorPage(new_server);
        else if (title == "root")
            parseRoot(new_server);
        else if (title == "index")
            parseIndex(new_server);
		else if (title == "location")
            parseLocation(new_server);
		else {

			throw std::out_of_range("invalid config - server");
        }
	}
	servers.push_back(new_server);
	tokens.pop();
}

void Config::parseListen(server &new_server) {
    std::string port = tokens.front();
    tokens.pop();
    if (port.back() != ';') {
        throw std::out_of_range("missing ';' after port number");
    }
    port.pop_back();
    std::stringstream ss(port);
    ss >> new_server.listen_port;
    if (ss.fail()) {
        throw std::out_of_range("invalid port number");
    }
}

void Config::parseServerName(server &new_server) {
    std::string name = tokens.front();
    tokens.pop();
    if (name.back() != ';') {
        throw std::out_of_range("missing ';' after server name");
    }
    name.pop_back();
    new_server.server_name = name;
}


void Config::parseErrorPage(server& new_server) {
    std::string errorCodeStr = tokens.front();
    tokens.pop();

    int errorCode;
    std::stringstream ss1(errorCodeStr);
    ss1 >> errorCode;
    if (ss1.fail()) {
        throw std::out_of_range("invalid error code");
    }

    std::string errorPage = tokens.front();
    tokens.pop();
    if (errorPage.back() != ';') {
        throw std::out_of_range("missing ';' after error page");
    }
    errorPage.pop_back();

    new_server.errorPages.insert(std::make_pair(errorCode, errorPage));
}

void Config::parseRoot(server &new_server) {
    std::string path = tokens.front();
    if (path.back() != ';') {
        throw std::out_of_range("missing ';' after root path");
    }
    path.pop_back();
    new_server.root_path = path;
    tokens.pop();
}

void Config::parseIndex(server &new_server) {
    std::string indexFile = tokens.front();
    if (indexFile.back() != ';') {
        throw std::out_of_range("missing ';' after index file");
    }
    indexFile.pop_back();
    new_server.index_file = indexFile;
    tokens.pop();
}

void Config::parseLocation(server& server) {
	
	location new_location;
	
    if (tokens.front() == "~") {
        tokens.pop();
        new_location.is_regex = true;
    } else {
        new_location.is_regex = false;
    }

    new_location.url = tokens.front();
    tokens.pop();

    if (tokens.front() != "{") {
        throw std::out_of_range("missing '{' after location");
    }
    tokens.pop();
	std::string buf;
	while (tokens.front() != "}") {
		std::string key = tokens.front();
        tokens.pop();
        if (key == "limit_except") {
            parseLimitExcept(new_location);
        } else if (key == "try_files") {
            parseTryFiles(new_location);
        } else if (key == "root") {
            parseRoot(new_location);
        } else if (key == "autoindex") {
            parseAutoIndex(new_location);
        } else if (key == "include") {
            parseInclude(new_location);
        } else if (key == "fastcgi_pass") {
            parseFastcgiPass(new_location);
        } else if (key == "fastcgi_param") {
            parseFastcgiParam(new_location);
        } else if (key == "return") {
            parseReturn(new_location);
        } else {
            std::cout << key << std::endl;
            throw std::out_of_range("invalid config - location");
        }
	}
	tokens.pop();
	server.locations.push_back(new_location);
}

void Config::parseLimitExcept(location& loc) {
    while (tokens.front().back() != ';') {
        loc.allowedMethod.push_back(tokens.front());
        tokens.pop();
    }
    std::string lastMethod = tokens.front();
    if (lastMethod.back() != ';') {
        throw std::out_of_range("missing ';' after last method");
    }
    lastMethod.pop_back();
    loc.allowedMethod.push_back(lastMethod);
    tokens.pop();
}


void Config::parseTryFiles(location& loc) {
    while (tokens.front().back() != ';') {
        loc.try_files.push_back(tokens.front());
        tokens.pop();
    }
    std::string lastFile = tokens.front();
    lastFile.pop_back(); // Removing trailing ';'
    loc.try_files.push_back(lastFile);
    tokens.pop();
}

void Config::parseRoot(location& loc) {
    std::string path = tokens.front();
    if (path.back() != ';') {
        throw std::out_of_range("missing ';' after root path");
    }
    path.pop_back();
    loc.root = path;
    tokens.pop();
}

void Config::parseAutoIndex(location& loc) {
    std::string value = tokens.front();
    value.pop_back();
    loc.autoindex = (value == "on");
    tokens.pop();
}

void Config::parseInclude(location& loc) {
    std::string file = tokens.front();
    file.pop_back();
    loc.include_file = file;
    tokens.pop();
}

void Config::parseFastcgiPass(location& loc) {
    std::string path = tokens.front();
    path.pop_back();
    loc.fastcgi_pass = path;
    tokens.pop();
}

void Config::parseFastcgiParam(location& loc) {
    std::string key = tokens.front();
    tokens.pop();
    std::string value = tokens.front();
    value.pop_back();
    loc.fastcgi_params[key] = value;
    tokens.pop();
}

void Config::parseReturn(location& loc) {
    std::stringstream ss(tokens.front());
    ss >> loc.return_code;
    if (ss.fail() || loc.return_code < 100 || loc.return_code > 599) {
        throw std::out_of_range("invalid return code");
    }
    tokens.pop();

    loc.return_url = tokens.front();
    if (loc.return_url.back() != ';') {
        throw std::out_of_range("missing ';' after return URL");
    }
    loc.return_url.pop_back();
    tokens.pop();

    if (loc.return_url.empty()) {
        throw std::out_of_range("return URL must not be empty");
    }
}

Config::~Config() {}
Config::Config(const Config& copy) {
    static_cast<void>(copy);
}

Config& Config::operator=(const Config& copy) {
	static_cast<void>(copy);
	return *this;
}

const std::vector<server>& Config::getServers() const {
    return servers;
}

int Config::getClientMaxBodySize() const {
    return clientMaxBodySize;
}