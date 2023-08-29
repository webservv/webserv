#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/_types/_size_t.h>

Request::Request():
    requestStr(""),
    requestLines(),
    method(OTHER),
    values(),
    haveHeader(false) {}

Request::Request(const Request& copy):
    requestStr(copy.requestStr),
    requestLines(copy.requestLines),
    method(copy.method),
    values(copy.values),
    haveHeader(copy.haveHeader) {}

Request& Request::operator=(const Request& copy) {
	requestStr = copy.requestStr;
	requestLines = copy.requestLines;
	method = copy.method;
	values = copy.values;

    haveHeader = copy.haveHeader;
	return *this;
}

Request::~Request() {}

Request::METHOD Request::getMethod(void) const {
	return method;
}

const std::string& Request::getStrMethod(void) const {
    return findValue("method");
}

const std::string& Request::getUrl(void) const {
	return findValue("url");
}

const std::string& Request::getVersion(void) const {
    return findValue("version");
}

static char tolower_char(unsigned char c) {
    return std::tolower(c);
}

const std::string& Request::findValue(const std::string& headerName) const {
    std::string lowerHeaderName = headerName;
    std::transform(lowerHeaderName.begin(), lowerHeaderName.end(), lowerHeaderName.begin(), tolower_char);
    std::map<std::string, std::string>::const_iterator it = values.find(lowerHeaderName);
    if (it != values.end()) {
        return it->second;
    }
    static const std::string emptyString = "";
    return emptyString;
}

const std::string& Request::getBody(void) const {
	return findValue("body");
}

void Request::addRequest(const std::string &request) {
    this->requestStr += request;
}

bool Request::isHeaderEnd(void) const {
    if (haveHeader)
        return true;
    size_t pos = requestStr.rfind("\r\n\r\n");
    if (pos != std::string::npos) {
        return true;
    } else {
        return false;
    }
}

bool Request::isRequestEnd(void) const {
    std::map<std::string, std::string>::const_iterator it = values.find("transfer-encoding");
    
    if (it != values.end() && it->second == "chunked") {
        it = values.find("body");
        if (it != values.end()) {
            const std::string&  body = it->second;
            for (size_t i = body.size() - 1; i >= 0; i--) {
                if (body[i] == '\r' || body[i] == '\n')
                    continue;
                else if (body[i] == '0')
                    return true;
                else
                    return false;
            }
        }
        return false;
    }
    it = values.find("content-length");
    if (it != values.end()) {
        size_t len = std::atoi(it->second.c_str());
        it = values.find("body");
        if (it != values.end() && it->second.size() == len)
            return true;
        else
            return false;
    }
    else
        return true;
}

int Request::getError(void) const {
    return error;
}
