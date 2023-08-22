#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/_types/_size_t.h>

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

Request::METHOD Request::getMethod() {
	return method;
}
const std::string& Request::getUrl() {
	return url;
}

static char tolower_char(unsigned char c) {
    return std::tolower(c);
}

const std::string& Request::getHeaderValue(const std::string& headerName) const {
    std::string lowerHeaderName = headerName;
    std::transform(lowerHeaderName.begin(), lowerHeaderName.end(), lowerHeaderName.begin(), tolower_char);
    std::map<std::string, std::string>::const_iterator it = headers.find(lowerHeaderName);
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
    std::map<std::string, std::string>::iterator it = headers.find("transfer-encoding");
    
    if (it != headers.end() && headers["transfer-encoding"] == "chunked") {
        if (!bodyLines.empty() && bodyLines.back()[0] == '0') {
            std::cout << "chunked end" << std::endl;
            return true;
        }
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
