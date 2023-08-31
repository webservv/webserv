
#include "Config.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>

Config::Config()
    : tokens()
    , servers()
    , clientMaxBodySize()
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
    , clientMaxBodySize()
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
        } else if (title == "client_max_body_size") {
            parseClientMaxBodySize();
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
    server new_server;
    std::string buf;
    bool hasListen = false;
    bool hasServerName = false;
    bool hasRoot = false;
    bool hasIndex = false;
    bool hasLocation = false;

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
            parseListen(new_server);
            hasListen = true;
        } else if (title == "server_name") {
            if (hasServerName) {
                throw std::out_of_range("duplicate server_name entry in server");
            }
            parseServerName(new_server);
            hasServerName = true;
        } else if (title == "error_page") {
            parseErrorPage(new_server);
        } else if (title == "root") {
            if (hasRoot) {
                throw std::out_of_range("duplicate root entry in server");
            }
            parseRoot(new_server);
            hasRoot = true;
        } else if (title == "index") {
            parseIndex(new_server);
            hasIndex = true;
        } else if (title == "location") {
            parseLocation(new_server);
            hasLocation = true;
        } else {
            throw std::out_of_range("invalid config - server");
        }
        if (tokens.empty()) {
            throw std::out_of_range("missing '}' in server block");
        }
    }

    if (!hasListen)
        new_server.listen_port = 8080;
    if (!hasIndex)
        new_server.index.push_back("index.html");
    if (!hasRoot)
        throw std::out_of_range("missing root directory of server");
    if (!hasLocation)
        throw std::out_of_range("missing location entry in server");

    servers.push_back(new_server);
    tokens.pop();
}

void Config::parseListen(server &new_server) {
    if (tokens.empty()) {
        throw std::out_of_range("Unexpected end of file: Expected a port number");
    }
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
    if (new_server.listen_port < 0 || new_server.listen_port > 65535) {
        throw std::out_of_range("port number out of range");
    }
}

void Config::parseServerName(server &new_server) {
    if (tokens.empty()) {
        throw std::out_of_range("Unexpected end of file: Expected a server name");
    }
    std::string name = tokens.front();
    tokens.pop();
    if (name.back() != ';') {
        throw std::out_of_range("missing ';' after server name");
    }
    name.pop_back();
    if (name.empty()) {
        throw std::out_of_range("server name must not be empty");
    } else if (name.front() == '.' || name.back() == '.') {
        throw std::out_of_range("server name must not start or end with '.'");
    } else if (name.find("..") != std::string::npos) {
        throw std::out_of_range("server name must not contain '..'");
    } else if (name.find(" ") != std::string::npos) {
        throw std::out_of_range("server name must not contain spaces");
    } else if (name.find("\t") != std::string::npos) {
        throw std::out_of_range("server name must not contain tabs");
    } else if (name.find("\r") != std::string::npos) {
        throw std::out_of_range("server name must not contain carriage returns");
    } else if (name.find("\n") != std::string::npos) {
        throw std::out_of_range("server name must not contain newlines");
    } else if (name.size() > 253) {
        throw std::out_of_range("server name must not be longer than 253 characters");
    }
    new_server.server_name = name;
}

void Config::parseRoot(server &new_server) {
    if (tokens.size() < 2) {
        throw std::out_of_range("root expects exactly 2 arguments");
    }
    std::string path = tokens.front();
    if (path.back() != ';') {
        throw std::out_of_range("missing ';' after root path");
    }
    path.pop_back();
    new_server.root = path;
    tokens.pop();
}

void Config::parseIndex(server &new_server) {
    if (tokens.empty()) {
        throw std::out_of_range("index expects at least one argument");
    }

    while (tokens.front().back() != ';') {
        new_server.index.push_back(tokens.front());
        tokens.pop();
        if (tokens.empty()) {
            throw std::out_of_range("missing ';' after last index");
        }
    }

    std::string lastIndex = tokens.front();
    lastIndex.pop_back();
    new_server.index.push_back(lastIndex);
    tokens.pop();
}

void Config::parseErrorPage(server& new_server) {
    if (tokens.size() < 2) {
        throw std::out_of_range("error_page expects at least 2 arguments");
    }
    std::string token = tokens.front();
    tokens.pop();

    std::vector<int> errorCodes;

    if (token == "/" || token.back() == ';') {
        throw std::out_of_range("missing error codes");
    }

    while (token.back() != ';') {
        int errorCode;
        std::stringstream ss1(token);
        ss1 >> errorCode;
        if (ss1.fail()) {
            throw std::out_of_range("invalid error code");
        }
        if (errorCode < 100 || errorCode > 599) {
            throw std::out_of_range("error code out of range");
        }
        errorCodes.push_back(errorCode);

        token = tokens.front();
        tokens.pop();
    }

    if (tokens.empty()) {
        throw std::out_of_range("missing error page");
    }
    std::string errorPage = token;

    if (errorPage[errorPage.size() - 1] != ';') {
        throw std::out_of_range("missing ';' after error page");
    }
    errorPage.erase(errorPage.size() - 1);

    if (errorPage.empty()) {
        throw std::out_of_range("error page cannot be empty");
    }

    for (std::vector<int>::iterator it = errorCodes.begin(); it != errorCodes.end(); ++it) {
        new_server.errorPages.insert(std::make_pair(*it, errorPage));
    }
}

void Config::parseClientMaxBodySize() {
    if (tokens.empty()) {
        throw std::out_of_range("Unexpected end of file: Expected a client_max_body_size");
    }
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
    } else if (valueStr.back() == 'K') {
        multiplier = 1024;
        valueStr.pop_back();
    } else if (valueStr.back() == 'G') {
        multiplier = 1024 * 1024 * 1024;
        valueStr.pop_back();
    } else if (valueStr.back() == 'B') {
        valueStr.pop_back();
    } else {
        throw std::out_of_range("invalid client_max_body_size, must end with B, K, M or G");
    }

    std::stringstream ss(valueStr);
    ss >> clientMaxBodySize;
    if (ss.fail() || clientMaxBodySize < 0) {
        throw std::out_of_range("invalid client_max_body_size, must be non-negative");
    }

    clientMaxBodySize *= multiplier;
}

void Config::parseLocation(server& server) {
    location new_location;
    if (tokens.empty()) {
        throw std::out_of_range("Unexpected end of file: Expected a location block");
    }
    new_location.url = tokens.front();
    tokens.pop();
    if (new_location.url.empty()) {
        throw std::out_of_range("URL pattern for location cannot be empty");
    }
    if (tokens.empty() || tokens.front() != "{") {
        throw std::out_of_range("missing '{' after location");
    }
    tokens.pop();

    if (tokens.empty()) {
        throw std::out_of_range("Unexpected end of file within location block");
    }

    while (!tokens.empty() && tokens.front() != "}") {
        std::string key = tokens.front();
        tokens.pop();

        if (key == "limit_except") {
            parseLimitExcept(new_location);
        } else if (key == "root") {
            parseRoot(new_location);
        } else if (key == "autoindex") {
            parseAutoIndex(new_location);
        } else if (key == "index") {
            parseIndex(new_location);
        } else if (key == "return") {
            parseReturn(new_location);
        } else {
            throw std::out_of_range("invalid config - location");
        }
    }
    
    if (tokens.empty() || tokens.front() != "}") {
        throw std::out_of_range("missing '}' in location block");
    }
    tokens.pop();

    if (new_location.index.empty())
        new_location.index.push_back("index.html");
    server.locations.push_back(new_location);
}

void Config::parseLimitExcept(location& loc) {
    if (tokens.size() < 2) {
        throw std::out_of_range("limit_except expects at least 2 arguments");
    }
    while (tokens.front().back() != ';') {
        std::string method = tokens.front();
        if (method != "GET" && method != "POST" && method != "DELETE") {
            throw std::out_of_range("Invalid method: " + method);
        }
        if (std::find(loc.allowedMethod.begin(), loc.allowedMethod.end(), method) != loc.allowedMethod.end()) {
            throw std::out_of_range("Duplicate method found: " + method);
        }
        loc.allowedMethod.push_back(method);
        tokens.pop();
    }
    std::string lastMethod = tokens.front();
    if (lastMethod.back() != ';') {
        throw std::out_of_range("missing ';' after last method");
    }
    lastMethod.pop_back();
    if (lastMethod != "GET" && lastMethod != "POST" && lastMethod != "DELETE") {
        throw std::out_of_range("Invalid method: " + lastMethod);
    }
    if (std::find(loc.allowedMethod.begin(), loc.allowedMethod.end(), lastMethod) != loc.allowedMethod.end()) {
        throw std::out_of_range("Duplicate method found: " + lastMethod);
    }
    loc.allowedMethod.push_back(lastMethod);
    tokens.pop();
}

void Config::parseRoot(location& loc) {
    if (tokens.size() < 2) {
        throw std::out_of_range("root expects exactly 2 arguments");
    }
    std::string path = tokens.front();
    if (path.back() != ';') {
        throw std::out_of_range("missing ';' after root path");
    }
    path.pop_back();
    loc.root = path;
    tokens.pop();
}

void Config::parseIndex(location& loc) {
    if (tokens.empty()) {
        throw std::out_of_range("index expects at least one argument");
    }

    while (tokens.front().back() != ';') {
        loc.index.push_back(tokens.front());
        tokens.pop();
        if (tokens.empty()) {
            throw std::out_of_range("missing ';' after last index");
        }
    }

    std::string lastIndex = tokens.front();
    lastIndex.pop_back();
    loc.index.push_back(lastIndex);
    tokens.pop();
}

void Config::parseAutoIndex(location& loc) {
    if (tokens.size() < 2) {
        throw std::out_of_range("autoindex expects exactly 2 arguments");
    }

    std::string value = tokens.front();
    tokens.pop();

    if (value.back() != ';') {
        throw std::out_of_range("missing ';' after autoindex value");
    }

    value.erase(value.size() - 1);

    if (value != "on" && value != "off") {
        throw std::out_of_range("invalid value for autoindex, expected 'on' or 'off'");
    }

    loc.autoindex = (value == "on");
}

void Config::parseReturn(location& loc) {
    if (tokens.size() < 3) {
        throw std::out_of_range("return expects exactly 3 arguments");
    }
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

const std::vector<Config::server>& Config::getServers() const {
    return servers;
}

int Config::getClientMaxBodySize() const {
    return clientMaxBodySize;
}