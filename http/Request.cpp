#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <sys/_types/_size_t.h>

static const size_t BUFFER_SIZE = 120000000;

Request::Request()
    : requestStr()
    , values()
    , body()
    , readPos(0)
    , chunkSize(0)
    , valueStart()
    , haveHeader(false)
    , haveBody(false) {
        requestStr.reserve(BUFFER_SIZE);
        body.reserve(BUFFER_SIZE);
    }

Request::Request(const Request& copy)
    : requestStr(copy.requestStr)
    , values(copy.values)
    , body(copy.body)
    , readPos(copy.readPos)
    , chunkSize(copy.chunkSize)
    , valueStart(copy.valueStart)
    , haveHeader(copy.haveHeader)
    , haveBody(copy.haveBody) {
        requestStr.reserve(BUFFER_SIZE);
        body.reserve(BUFFER_SIZE);
    }

Request& Request::operator=(const Request& copy) {
	requestStr = copy.requestStr;
	values = copy.values;
    body = copy.body;
    readPos = copy.readPos;
    chunkSize = copy.chunkSize;
    valueStart = copy.valueStart;
    haveHeader = copy.haveHeader;
    haveBody = copy.haveBody;
    haveHeader = copy.readPos;
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

const std::vector<char>& Request::getRequestStr(void) const {
    return requestStr;
}

const std::string& Request::getMethod(void) const {
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

void Request::addRequest(const Buffer& input) {
    requestStr.insert(requestStr.end(), input.begin(), input.end());
}

bool Request::isRequestEnd(void) const {
    return haveHeader && haveBody;
}