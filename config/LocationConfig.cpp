#include "LocationConfig.hpp"
#include "ServerConfig.hpp"
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

LocationConfig::LocationConfig()
	: URL()
	, root()
	, index()
	, limitExcept()
	, returnCode()
	, returnURL()
	, CgiPath()
	, CgiLimit()
	, autoIndex(false)
	, clientMaxBodySize() {}

LocationConfig::LocationConfig(const ServerConfig& server)
	: URL()
	, root(server.getRoot())
	, index(server.getIndex())
	, limitExcept()
	, returnCode(server.getReturnCode())
	, returnURL(server.getReturnURL())
	, CgiPath()
	, CgiLimit()
	, autoIndex(false)
	, clientMaxBodySize(server.getClientMaxBodySize()) {}

LocationConfig::LocationConfig(const LocationConfig& src)
	: URL(src.URL)
	, root(src.root)
	, index(src.index)
	, limitExcept(src.limitExcept)
	, returnCode(src.returnCode)
	, returnURL(src.returnURL)
	, CgiPath(src.CgiPath)
	, CgiLimit(src.CgiLimit)
	, autoIndex(src.autoIndex)
	, clientMaxBodySize(src.clientMaxBodySize) {}

LocationConfig::~LocationConfig() {}

LocationConfig& LocationConfig::operator=(const LocationConfig &src) {
	URL = src.URL;
	root = src.root;
	index = src.index;
	limitExcept = src.limitExcept;
	returnCode = src.returnCode;
	returnURL = src.returnURL;
	CgiPath = src.CgiPath;
	CgiLimit = src.CgiLimit;
	autoIndex = src.autoIndex;
	clientMaxBodySize = src.clientMaxBodySize;
	return *this;
}

bool LocationConfig::isValidMethod(const std::string& method) const {
	if (method != "GET" &&
		method != "POST" &&
		method != "DELETE" &&
		method != "PUT")
		return false;
	return true;
}

bool LocationConfig::isNumber(const std::string& str) const {
    for (size_t i = 0; i < str.size(); ++i) {
        if (!std::isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

void LocationConfig::parseURL(std::queue<std::string> &tokens) {
	if (tokens.empty()) {
        throw std::out_of_range("parseURL: Unexpected end of file: Expected a location block");
    }
	URL = tokens.front();
	tokens.pop();
	if (URL.empty()) {
        throw std::out_of_range("parseURL: URL pattern for location cannot be empty");
    }
	if (tokens.empty() || tokens.front() != "{") {
        throw std::out_of_range("parseURL: missing '{' after location");
    }
    tokens.pop();
	if (tokens.empty()) {
        throw std::out_of_range("parseURL: Unexpected end of file within location block");
    }
}

void LocationConfig::parseRoot(std::queue<std::string> &tokens) {
	if (tokens.empty()) {
        throw std::out_of_range("parseRoot: root expects exactly 2 arguments");
    }
    std::string path = tokens.front();
    if (tokens.front().back() != ';') {
        throw std::out_of_range("missing ';' after root path");
    }
    root = tokens.front();
	tokens.pop();
	root.pop_back();
}

void LocationConfig::parseIndex(std::queue<std::string> &tokens) {
	if (tokens.empty()) {
        throw std::out_of_range("parseIndex: index expects at least one argument");
    }
	index.clear();
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

void LocationConfig::parseLimitExcept(std::queue<std::string> &tokens) {
	if (tokens.empty()) {
        throw std::out_of_range("parseLimitExcept: limit_except expects at least 2 arguments");
    }
	while (tokens.front().back() != ';') {
        const std::string&	method = tokens.front();
        if (!isValidMethod(method)) {
            throw std::out_of_range("parseLimitExcept: Invalid method: " + method);
        }
        if (std::find(limitExcept.begin(), limitExcept.end(), method) != limitExcept.end()) {
            throw std::out_of_range("parseLimitExcept: Duplicate method found: " + method);
        }
        limitExcept.push_back(method);
        tokens.pop();
		if (tokens.empty())
			throw std::out_of_range("parseLimitExcept: missing ';' after last index");
    }
	limitExcept.push_back(tokens.front());
	tokens.pop();
	limitExcept.back().pop_back();
	if (!isValidMethod(limitExcept.back())) {
        throw std::out_of_range("parseLimitExcept: Invalid method: " + limitExcept.back());
    }
	if (std::find(limitExcept.begin(), limitExcept.end(), limitExcept.back()) != limitExcept.end() - 1) {
        throw std::out_of_range("parseLimitExcept: Duplicate method found: " + limitExcept.back());
    }
}

void LocationConfig::parseAutoIndex(std::queue<std::string> &tokens) {
	if (tokens.empty()) {
        throw std::out_of_range("parseAutoIndex: autoindex expects exactly 2 arguments");
    }
	std::string value = tokens.front();
    tokens.pop();
	if (value.back() != ';')
		throw std::out_of_range("parseAutoIndex: missing ';' after autoindex value");
	value.pop_back();
	if (value != "on" && value != "off") {
        throw std::out_of_range("parseAutoIndex: invalid value for autoindex, expected 'on' or 'off'");
    }
	autoIndex = (value == "on");
}

void LocationConfig::parseReturn(std::queue<std::string> &tokens) {
	std::stringstream	ss;

	if (tokens.size() < 2)
		throw std::out_of_range("parseReturn: return expects exactly 3 arguments");
	ss << tokens.front();
	tokens.pop();
	ss >> returnCode;
	if (ss.fail())
		throw std::out_of_range("parseReturn: stringstream Error");
	if (returnCode < 100 || returnCode > 599) {
        throw std::out_of_range("parseReturn: invalid return code");
    }
	returnURL = tokens.front();
	tokens.pop();
	if (returnURL.back() != ';')
		throw std::out_of_range("parseReturn: missing ';' after return URL");
	returnURL.pop_back();
	if (returnURL.empty())
		throw std::out_of_range("parseReturn: return URL must not be empty");
}

void LocationConfig::parseCgiPath(std::queue<std::string> &tokens) {
	if (tokens.empty())
        throw std::out_of_range("parseCgiPath: cgi_path expects exactly one argument");
	CgiPath = tokens.front();
	tokens.pop();
	if (CgiPath.back() != ';')
		throw std::out_of_range("parseCgiPath: missing ';'");
	CgiPath.pop_back();
}

void LocationConfig::parseCgiLimit(std::queue<std::string> &tokens) {
	if (tokens.empty())
        throw std::out_of_range("parseCgiLimit: no value");
	while (tokens.front().back() != ';') {
        CgiLimit.push_back(tokens.front());
        tokens.pop();
		if (!isValidMethod(CgiLimit.back()))
			throw std::out_of_range("parseCgiLimit: invalid method");
        if (tokens.empty()) {
            throw std::out_of_range("parseCgiLimit: missing ';' after last index");
        }
    }
	CgiLimit.push_back(tokens.front());
    tokens.pop();
    CgiLimit.back().pop_back();
}

void LocationConfig::parseClientMaxBodySize(std::queue<std::string> &tokens) {
	std::stringstream	ss;
	std::string			value;
	char				unit;
	size_t				multiplier;
	int					lastDigit = -1;

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
	else if (unit == 'B' || std::isdigit(unit)) {
		multiplier = 1;
		lastDigit = unit - '0';
	}
	else
		throw std::out_of_range("parseClientMaxBodySize: invalid client_max_body_size, \
            must end with 'B', 'K', 'M' or 'G' or be a number");
	value.pop_back();
	if (!isNumber(value))
		throw std::out_of_range("parseClientMaxBodySize: invalid client_max_body_size, must be a number");
	ss << value;
	ss >> clientMaxBodySize;
	if (lastDigit != -1)
		clientMaxBodySize = clientMaxBodySize * 10 + lastDigit;
	clientMaxBodySize *= multiplier;
}

const std::string& LocationConfig::getURL(void) const {
	return URL;
}

const std::string& LocationConfig::getRoot(void) const {
	return root;
}

const std::vector<std::string>& LocationConfig::getIndex(void) const {
	return index;
}

const std::vector<std::string>& LocationConfig::getLimitExcept(void) const {
	return limitExcept;
}

int LocationConfig::getReturnCode(void) const {
	return returnCode;
}

const std::string& LocationConfig::getReturnURL(void) const {
	return returnURL;
}

const std::string& LocationConfig::getCgiPath(void) const {
	return CgiPath;
}

const std::vector<std::string>& LocationConfig::getCgiLimit(void) const {
	return CgiLimit;
}

bool LocationConfig::getAutoIndex(void) const {
	return autoIndex;
}

size_t LocationConfig::getClientMaxBodySize(void) const {
	return clientMaxBodySize;
}