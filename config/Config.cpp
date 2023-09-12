#include "Config.hpp"
#include "LocationConfig.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <stdexcept>

Config::Config()
    : tokens()
    , servers()
    , hasHTTP() {}

Config::Config(const Config& copy) {
    static_cast<void>(copy);
}

Config& Config::operator=(const Config& copy) {
	static_cast<void>(copy);
	return *this;
}

Config::Config(const std::string& config_file)
    : tokens()
    , servers()
    , hasHTTP(false)
    {
    std::fstream configParser;
    configParser.open(config_file.c_str());
    if (!configParser.is_open()) {
        throw std::runtime_error("config file open error");
    }
    parseLine(configParser);
    while (!tokens.empty()) {
        std::string token = tokens.front();
        tokens.pop();

        if (token == "http") {
            if (hasHTTP) {
                throw std::runtime_error("http block already exists");
            }
            hasHTTP = true;

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

Config::~Config() {}

void Config::parseLine(std::fstream& configParser) {
	std::string line;
	while (std::getline(configParser, line)) {
		size_t commentPos = line.find('#');
		if (commentPos != std::string::npos) {
			line.resize(commentPos);
		}
		trim(line);
		if (line.empty())
			continue;
		if (line.back() != ';' && line.back() != '{' && line.back() != '}') {
			throw std::out_of_range("missing ';' after " + line);
		}
		tokenization(line);
    }
}

void Config::parseHTTP(void) {
    bool hasServer = false;

    if (tokens.empty()) {
        throw std::out_of_range("Unexpected end of file: Expected a http block");
    }
    while (!tokens.empty() && tokens.front() != "}") {
        std::string title = tokens.front();
        tokens.pop();

        if (title == "server") {
            hasServer = true;
            parseServer();
        } else {
            throw std::out_of_range("invalid config - http");
        }
    }

    if (tokens.empty()) {
        throw std::out_of_range("missing '}' in http block");
    }

    if (!hasServer) {
        throw std::out_of_range("missing server block in http");
    }

    tokens.pop();
}

void Config::parseServer(void) {
    ServerConfig    newServer;
    std::string     buf;
    bool            hasListen = false;
    bool            hasServerName = false;
    bool            hasRoot = false;

    if (tokens.empty()) {
        throw std::out_of_range("Unexpected end of file: Expected a server block");
    }
    if (tokens.front() == "{") {
        tokens.pop();
    } else {
        throw std::out_of_range("missing '{' after server");
    }

    while (!tokens.empty() && tokens.front() != "}") {
        std::string title = tokens.front();
        tokens.pop();

        if (title == "listen") {
            if (hasListen) {
                throw std::out_of_range("duplicate listen entry in server");
            }
            newServer.parseListenPort(tokens);
            hasListen = true;
        } else if (title == "server_name") {
            if (hasServerName) {
                throw std::out_of_range("duplicate server_name entry in server");
            }
            newServer.parseServerName(tokens);
            hasServerName = true;
        } else if (title == "error_page") {
            newServer.parseErrorPages(tokens);
        } else if (title == "alias") {
            if (hasRoot) {
                throw std::out_of_range("duplicate root entry in server");
            }
            newServer.parseAlias(tokens);
            hasRoot = true;
        } else if (title == "index") {
            newServer.parseIndex(tokens);
        } else if (title == "client_max_body_size") {
            newServer.parseClientMaxBodySize(tokens);
        } else if (title == "location") {
            newServer.parseLocation(tokens);
        } else {
            throw std::out_of_range("invalid config - server");
        }
        if (tokens.empty()) {
            throw std::out_of_range("missing '}' in server block");
        }
    }
    if (newServer.getIndex().empty())
        newServer.addIndex("index.html");
    if (newServer.getListenPort() == 0)
        throw std::out_of_range("parseServer: missing listen port");
    if (newServer.getAlias().empty())
        throw std::out_of_range("missing root directory of server");
    if (newServer.getLocations().empty())
        throw std::out_of_range("missing location entry in server");
    servers.push_back(newServer);
    tokens.pop();
}

void Config::trim(std::string &str) const {
    std::string::const_iterator start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        ++start;
    }

    if (start == str.end()) {
        str.clear();
        return;
    }

    std::string::const_iterator end = str.end();
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

const std::vector<ServerConfig>& Config::getServers() const {
    return servers;
}
