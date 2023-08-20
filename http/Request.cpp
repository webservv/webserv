
#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/_types/_size_t.h>
#include <vector>

Request::Request():
    requestStr(""),
    requestLines(),
    method(OTHER),
    url(""),
    version(""),
    headers(),
    bodyLines(),
    body(""),
    haveHeader(false) {}

Request::Request(const Request& copy):
    requestStr(copy.requestStr),
    requestLines(copy.requestLines),
    method(copy.method),
    url(copy.url),
    version(copy.version),
    headers(copy.headers),
    bodyLines(copy.bodyLines),
    body(copy.body),
    haveHeader(copy.haveHeader) {}

Request& Request::operator=(const Request& copy) {
	requestStr = copy.requestStr;
	requestLines = copy.requestLines;
	method = copy.method;
	url = copy.url;
	version = copy.version;
	headers = copy.headers;
    bodyLines = copy.bodyLines;
	body = copy.body;
    haveHeader = copy.haveHeader;
	return *this;
}

Request::~Request() {}

void Request::parseMethod(std::string& line) {
    size_t space = line.find(' ');
    if (space == std::string::npos)
        throw std::out_of_range("invalid http, request line!1");
    std::string methodString = line.substr(0, space);
    line = line.substr(space + 1);

    if (methodString == "GET")
        method = GET;
    else if (methodString == "POST")
        method = POST;
    else if (methodString == "DELETE")
        method = DELETE;
    else
        throw std::out_of_range("invalid http, request line!2");
}

void Request::parseURL(std::string& line) {
    size_t space = line.find(' ');
    if (space == std::string::npos)
        throw std::out_of_range("invalid http, request line!3");
    url = line.substr(0, space);
}

void Request::parseVersion(std::string& line, size_t space) {
    version = line.substr(space + 1);
    if (version != "HTTP/1.1")
        throw std::out_of_range("invalid http, request line!4");
}

void Request::parseRequestLine() {
    if (requestLines.empty())
        throw std::out_of_range("invalid http, empty request line!");
    std::string line = requestLines.front();
    requestLines.pop();

    parseMethod(line);
    size_t space = line.find(' ');
    parseURL(line);
    parseVersion(line, space);
}

void Request::parseKeyValues(void) {
	while (!requestLines.empty()) {
		std::string line = requestLines.front();
		requestLines.pop();
		if (line.size() == 0)
			break;
		size_t index = line.find(": ");
		if (index == std::string::npos || index + 2 >= line.size())
			throw std::out_of_range("invalid http, header!");
		headers[line.substr(0, index)] = line.substr(index + 2);
	}
}

void Request::parseBody(void) {
    addRequestLines();
	while (!requestLines.empty()) {
        bodyLines.push_back(requestLines.front());
		requestLines.pop();
	}
    while (!bodyLines.empty() && bodyLines.back() == "") {
        bodyLines.pop_back();
    }
    for (std::vector<std::string>::iterator it = bodyLines.begin(); it != bodyLines.end(); it++) {
        body += *it;
    }
}

Request::METHOD Request::getMethod() {
	return method;
}
const std::string& Request::getUrl() {
	return url;
}

const std::string& Request::getHeaderValue(const std::string& headerName) const {
    std::unordered_map<std::string, std::string>::const_iterator it = headers.find(headerName);
    if (it != headers.end()) {
        return it->second;
    }
    static const std::string emptyString = "";
    return emptyString;
}

const std::vector<std::string>& Request::getBodyLines(void) const {
    return bodyLines;
}

const std::string& Request::getBody(void) const {
	return body;
}

void Request::addRequest(const std::string &request) {
    this->requestStr += request; 
}

bool Request::isHeaderEnd(void) const {
    size_t pos = requestStr.rfind("\r\n\r\n");
    if (pos != std::string::npos) {
        return true;
    } else {
        return false;
    }
}

bool Request::isRequestEnd(void) {
    std::unordered_map<std::string, std::string>::iterator it = headers.find("Transfer-Encoding");
    
    if (it != headers.end() && headers["Transfer-Encoding"] == "chunked") {
        if (!bodyLines.empty() && bodyLines.back()[0] == '0')
            return true;
        return false;
    }
    it = headers.find("Content-Length");
    if (it != headers.end()) {
        size_t len = std::atoi(headers["Content-Length"].c_str());
        if (body.length() == len)
            return true;
        else
            return false;
    }
    else
        return true;
}

void Request::parseHeader(void) {
    if (haveHeader)
		return;
    addRequestLines();
    parseRequestLine();
    parseKeyValues();
    haveHeader = true;
}

void Request::addRequestLines(void) {
    std::string line;
    std::stringstream parser(requestStr);

	while (std::getline(parser, line)) {
		if (line.back() == '\r')
			line.pop_back();
		requestLines.push(line);
	}
    requestStr.clear();
}