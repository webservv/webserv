#include "Request.hpp"
#include <cctype>
#include <ctime>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/_types/_size_t.h>
#include <vector>
#include "Router.hpp"

static const std::string POST_URL = "/cgi/index.php";

bool Request::getValueSize(const std::string& spacer) {
    const size_t    searchSize = requestStr.size() - (spacer.size() - 1);
    size_t          count;

    for (size_t i = readPos; i < searchSize; ++i) {
        count = 0;
        for (size_t j = 0; j < spacer.size(); ++j) {
            if (spacer[j] != requestStr[i + j])
                break;
            ++count;
        }
        if (count == spacer.size()) {
            readPos = i;
            return true;
        }
    }
    return false;
}

bool Request::splitHeader(const std::string& spacer) {
    size_t                      copySize;
    std::string::const_iterator start;
    std::string::const_iterator end;

    if (!getValueSize(spacer))
        return false;
    copySize = readPos - valueStart;
    readPos += spacer.size();
    start = requestStr.begin() + valueStart;
    end = start +copySize;
    valueBuffer.assign(start, end);
    return true;
}

bool Request::parseRequestLine() {
    if (values.find("method") == values.end()) {
        if (!splitHeader(" "))
            return false;
        if (isLineEnd)
            throw Router::ErrorException(400, "parseRequestLine: invalid request");
        values["method"] = valueBuffer;
        valueBuffer.clear();
        valueStart = readPos;
    }
    if (values.find("url") == values.end()) {
        if (!splitHeader(" "))
            return false;
        if (isLineEnd)
            throw Router::ErrorException(400, "parseRequestLine: invalid request");
        values["url"] = valueBuffer;
        valueBuffer.clear();
        valueStart = readPos;
    }
    if (values.find("version") == values.end()) {
        if (!splitHeader("\r\n"))
            return false;
        values["version"] = valueBuffer;
        valueBuffer.clear();
        valueStart = readPos;
    }
    return true;
}

bool Request::parseKeyValues(void) {
    size_t  spacer;

    while (splitHeader("\r\n")) {
        if (valueBuffer.empty())
            return true;
        spacer = valueBuffer.find(':');
        if (spacer == std::string::npos)
            throw Router::ErrorException(400, "parseKeyValues: missing ':'");
        values[valueBuffer.substr(0, spacer)] = valueBuffer.substr(spacer + 2, -1);
    }
    return false;
}

void Request::parseHeader(void) {
    if (haveHeader)
        return;
    if (!parseRequestLine())
        return;
    if (!parseKeyValues())
        return;
    haveHeader = true;
}

// static void timeStamp(int i) {
//     std::time_t Time = std::time(NULL);
//     std::string timeStr = std::ctime(&Time);
//     std::cout << "Time" << i << " : " << timeStr << std::endl;
// }

void Request::parseBody(void) {
    std::map<std::string, std::string>::const_iterator it = values.find("transfer-encoding");
    if (it != values.end() && it->second == "chunked") {
        while (readPos != requestStr.size())
            parseChunkedBody();
        }
    else {
        std::vector<char>::iterator st = requestStr.begin() + readPos;
        if (st < requestStr.end()) {
            body.insert(body.end(), st, requestStr.end());
            readPos = requestStr.size();
        }
        it = values.find("content-length");
        if (it == values.end()) {
            haveBody = true;
            return;
        }
        size_t len = std::stol(it->second.c_str());
        if (body.size() == len)
            haveBody = true;
    }
}

size_t Request::hexToDecimal(char digit) const {
    if (digit >= '0' && digit <= '9')
        return digit - '0';
    if (digit >= 'a' && digit <= 'f')
        return 10 + (digit - 'a');
    if (digit >= 'A' && digit <= 'F')
        return 10 + (digit - 'A');
    return -1;
}

void Request::skipCRLF(void) {
    if (requestStr.size() - readPos >= 2) {
        if (requestStr[readPos] == '\r' && requestStr[readPos + 1] == '\n')
            readPos += 2;
    }
}

bool Request::parseChunkSize(void) {
    size_t  hexDigit;

    for (; readPos < requestStr.size() - 1; ++readPos) {
        if ((requestStr[readPos] == '\r' && requestStr[readPos + 1] == '\n')) {
            readPos += 2;
            valueStart = readPos;
            return true;
        }
        hexDigit = hexToDecimal(requestStr[readPos]);
        if (hexDigit == static_cast<size_t>(-1))
            throw Router::ErrorException(400, "parseChunkSize: invalid chunk size");
        chunkSize =  chunkSize * 16 + hexDigit;
    }
    return false;
}

// static void timeStamp(int i) {
//     std::time_t Time = std::time(NULL);
//     std::string timeStr = std::ctime(&Time);
//     std::cout << "Time" << i << " : " << timeStr << std::endl;
// }

void Request::parseChunkedBody(void) {
    size_t                              copySize;
    size_t                              bodySize;
    std::vector<char>::const_iterator   start;
    std::vector<char>::const_iterator   end;

    if (chunkSize == 0) {
        if (!parseChunkSize())
            return;
    }
    if (chunkSize == 0) {
        skipCRLF();
        haveBody = true;
        return;
    }
    bodySize = requestStr.size() - valueStart;
    if (bodySize < chunkSize) {
        copySize = bodySize;
        readPos = requestStr.size();
        chunkSize -= bodySize;
    }
    else {
        copySize = chunkSize;
        readPos += chunkSize;
        chunkSize = 0;
    }
    start = requestStr.begin() + valueStart;
    end = start + copySize;
    body.insert(body.end(), start, end);
    skipCRLF();
}

void Request::parse() {
    parseHeader();
    parseBody();
}
