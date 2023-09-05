#include "ServerConfig.hpp"
#include <queue>
#include <sstream>
#include <stdexcept>

ServerConfig::ServerConfig()
	: serverName()
	, listenPort()
	, root("index.html")
	, errorPages()
	, index()
	, clientMaxBodySize(-1)
	, returnCode()
	, returnURL() {}

ServerConfig::ServerConfig(const ServerConfig& src)
	: serverName(src.serverName)
	, listenPort(src.listenPort)
	, root(src.root)
	, errorPages(src.errorPages)
	, index(src.index)
	, clientMaxBodySize(src.clientMaxBodySize)
	, returnCode(src.returnCode)
	, returnURL(src.returnURL) {}

ServerConfig::~ServerConfig() {}

ServerConfig& ServerConfig::operator=(const ServerConfig &src) {
	serverName = src.serverName;
	listenPort = src.listenPort;
	root = src.root;
	errorPages = src.errorPages;
	index = src.index;
	clientMaxBodySize = src.clientMaxBodySize;
	returnCode = src.returnCode;
	returnURL = src.returnURL;
	return *this;
}

bool ServerConfig::isNumber(const std::string& str) const {
    for (size_t i = 0; i < str.size(); ++i) {
        if (!std::isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

void ServerConfig::parseServerName(std::queue<std::string> &tokens) {
	if (tokens.empty())
        throw std::out_of_range("parseServerName: Unexpected end of file: Expected a server name");
	serverName = tokens.front();
	tokens.pop();
	if (serverName.empty())
		throw std::out_of_range("parseServerName: server name must not be empty");
	if (serverName.back() != ';')
		throw std::out_of_range("parseServerName: missing ';' after server name");
	serverName.pop_back();
	if (serverName.front() == '.' || serverName.back() == '.')
        throw std::out_of_range("parseServerName: server name must not start or end with '.'");
	if (serverName.find("..") != std::string::npos)
        throw std::out_of_range("parseServerName: server name must not contain '..'");
	if (serverName.find(" ") != std::string::npos)
        throw std::out_of_range("parseServerName: server name must not contain spaces");
	if (serverName.find("\t") != std::string::npos)
        throw std::out_of_range("parseServerName: server name must not contain tabs");
	if (serverName.find("\r") != std::string::npos)
        throw std::out_of_range("parseServerName: server name must not contain carriage returns");
	if (serverName.find("\n") != std::string::npos)
        throw std::out_of_range("parseServerName: server name must not contain newlines");
	if (serverName.size() > 253)
        throw std::out_of_range("parseServerName: server name must not be longer than 253 characters");
}

void ServerConfig::parseListenPort(std::queue<std::string> &tokens) {
	std::stringstream	ss;
	std::string			portStr;

	if (tokens.empty())
        throw std::out_of_range("parseListenPort: Unexpected end of file: Expected a port number");
	portStr = tokens.front();
	tokens.pop();
	if (portStr.back() != ';')
		throw std::out_of_range("parseListenPort: missing ';' after port number");
	portStr.pop_back();
	ss << portStr;
	ss >> listenPort;
	if (ss.fail())
		throw std::out_of_range("parseListenPort: invalid port number");
	if (listenPort < 0 || listenPort > 65535)
		throw std::out_of_range("parseListenPort: port number out of range");
}

void ServerConfig::parseRoot(std::queue<std::string> &tokens) {
	if (tokens.empty())
        throw std::out_of_range("parseRoot: no value");
	root = tokens.front();
	tokens.pop();
	if (root.back() != ';')
		throw std::out_of_range("parseRoot: missing ';' after root path");
	root.pop_back();
}

void ServerConfig::parseIndex(std::queue<std::string>& tokens) {
	if (tokens.empty())
        throw std::out_of_range("parseIndex: index expects at least one argument");
	while (tokens.front().back() != ';') {
        index.push_back(tokens.front());
        tokens.pop();
        if (tokens.empty()) {
            throw std::out_of_range("parseIndex: missing ';' after last index");
        }
    }
	index.push_back(tokens.front());
	tokens.pop();
	index.back().pop_back();
}

void ServerConfig::parseLocation(std::queue<std::string> &tokens) {
	LocationConfig  newLocation(*this);

    newLocation.parseURL(tokens);
    while (!tokens.empty() && tokens.front() != "}") {
        std::string key = tokens.front();
        tokens.pop();
        if (key == "root") {
            newLocation.parseRoot(tokens);
        } else if (key == "index") {
            newLocation.parseIndex(tokens);
        } else if (key == "limit_except") {
            newLocation.parseLimitExcept(tokens);
        } else if (key == "autoindex") {
            newLocation.parseAutoIndex(tokens);
        } else if (key == "return") {
            newLocation.parseReturn(tokens);
        } else if (key == "cgi_path") {
            newLocation.parseCgiPath(tokens);
        } else if (key == "cgi_limit") {
            newLocation.parseCgiLimit(tokens);
        } else if (key == "client_max_body_size") {
            newLocation.parseClientMaxBodySize(tokens);
        } else {
            throw std::out_of_range("parseLocation: invalid config - location");
        }
    }
    if (tokens.empty() || tokens.front() != "}") {
        throw std::out_of_range("missing '}' in location block");
    }
    tokens.pop();
    locations.push_back(newLocation);
}

void ServerConfig::parseErrorPages(std::queue<std::string> &tokens) {
	std::stringstream	ss;
	std::string			errorCodeStr;
	std::string			URL;
	int					errorCode;

	if (tokens.size() < 2)
        throw std::out_of_range("parseErrorPages: error_page expects at least 2 arguments");
	errorCodeStr = tokens.front();
	tokens.pop();
	if (!isNumber(errorCodeStr))
		throw std::out_of_range("parseErrorPages: invalid error code");
	ss << errorCodeStr;
	ss >> errorCode;
	if (ss.fail())
		throw std::out_of_range("parseErrorPages: invalid error code");
	if (errorCode < 100 | errorCode > 599)
		throw std::out_of_range("parseErrorPages: error code out of range");
	URL = tokens.front();
	tokens.pop();
	if (URL.back() != ';')
		throw std::out_of_range("parseErrorPages: missing ';'");
	URL.pop_back();
	errorPages[errorCode] = URL;
}

void ServerConfig::parseClientMaxBodySize(std::queue<std::string> &tokens) {
	std::stringstream	ss;
	std::string			value;
	char				unit;
	size_t				multiplier;

	if (tokens.empty())
        throw std::out_of_range("ParseClientMaxBodySize: Unexpected end of file: Expected a client_max_body_size");
	value = tokens.front();
	tokens.pop();
	if (value.back() != ';')
		throw std::out_of_range("parseClientMaxBodySize: missing ';' after client_max_body_size");
	value.pop_back();
	unit = value.back();
	if (unit == 'G')
		multiplier = 1024 * 1024 * 1024;
	else if (unit == 'M')
		multiplier = 1024 * 1024;
	else if (unit == 'K')
		multiplier = 1024;
	else if (unit == 'B' || std::isdigit(unit))
		multiplier = 1;
	else
		throw std::out_of_range("parseClientMaxBodySize: invalid client_max_body_size, \
            must end with 'B', 'K', 'M' or 'G' or be a number");
	value.pop_back();
	if (!isNumber(value))
		throw std::out_of_range("parseClientMaxBodySize: invalid client_max_body_size, must be a number");
	ss << value;
	ss >> clientMaxBodySize;
	clientMaxBodySize *= multiplier;
}

const std::string& ServerConfig::getServerName(void) const {
	return serverName;
}

int ServerConfig::getListenPort(void) const {
	return listenPort;
}

const std::string& ServerConfig::getRoot(void) const {
	return root;
}

const std::vector<LocationConfig>& ServerConfig::getLocations(void) const {
	return locations;
}

const std::map<int, std::string>& ServerConfig::getErrorPages(void) const {
	return errorPages;
}

const std::vector<std::string>& ServerConfig::getIndex(void) const {
	return index;
}

size_t ServerConfig::getClientMaxBodySize(void) const {
	return clientMaxBodySize;
}

int ServerConfig::getReturnCode(void) const {
	return returnCode;
}

const std::string& ServerConfig::getReturnURL(void) const {
	return returnURL;
}