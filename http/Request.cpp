#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <sys/_types/_size_t.h>

Request::Request():
    requestStr(),
    requestLines(),
    method(OTHER),
    values(),
    body(),
    haveHeader(false),
    haveBody(false),
    error(0),
    bodyPos(0) {
        static const size_t BUFFER_SIZE = 10000000;
        requestStr.reserve(BUFFER_SIZE);
    }

Request::Request(const Request& copy):
    requestStr(copy.requestStr),
    requestLines(copy.requestLines),
    method(copy.method),
    values(copy.values),
    body(),
    haveHeader(copy.haveHeader),
    haveBody(copy.haveBody),
    error(copy.error),
    bodyPos(0) {}

Request& Request::operator=(const Request& copy) {
	requestStr = copy.requestStr;
	requestLines = copy.requestLines;
	method = copy.method;
	values = copy.values;
    body = copy.body;
    haveHeader = copy.haveHeader;
    haveBody = copy.haveBody;
    error = copy.error;
    haveHeader = copy.bodyPos;
	return *this;
}

Request::~Request() {}

//use until making KMP function
size_t Request::findHeaderEnd(void) const {
    size_t  ret = -1;

    for (size_t i = 0; i < requestStr.size() - 3; ++i) {
        if (requestStr[i] == '\r' && requestStr[i + 1] == '\n'
        &&  requestStr[i + 2] == '\r' && requestStr[i + 3] == '\n') {
            ret = i;
            break;
        }
    }
    return ret;
}

Request::METHOD Request::getMethod(void) const {
	return method;
}

const std::vector<char>& Request::getRequestStr(void) const {
    return requestStr;
}

const std::string& Request::getStrMethod(void) const {
    return findValue("method");
}

const std::string& Request::getURL(void) const {
	return findValue("url");
}

const std::vector<char>& Request::getBody(void) const {
	return body;
}

const std::string& Request::getVersion(void) const {
    return findValue("version");
}

int Request::getError(void) const {
    return error;
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

void Request::addRequest(const std::vector<char>& input) {
    requestStr.insert(requestStr.end(), input.begin(), input.end());
}

bool Request::isHeaderEnd(void) {
    if (haveHeader)
        return true;
    size_t pos = findHeaderEnd();
    if (pos != std::string::npos) {
        bodyPos = pos + 4;
        return true;
    } else {
        return false;
    }
}

bool Request::isRequestEnd(void) const {
    return haveHeader && haveBody;
}