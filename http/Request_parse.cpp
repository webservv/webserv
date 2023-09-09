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
    valueStart = readPos;
    return true;
}

bool Request::parseRequestLine() {
    if (values.find("method") == values.end()) {
        if (!splitHeader(" "))
            return false;
        values["method"] = valueBuffer;
        valueBuffer.clear();
    }
    if (values.find("url") == values.end()) {
        if (!splitHeader(" "))
            return false;
        values["url"] = valueBuffer;
        valueBuffer.clear();
    }
    if (values.find("version") == values.end()) {
        if (!splitHeader("\r\n"))
            return false;
        values["version"] = valueBuffer;
        valueBuffer.clear();
    }
    return true;
}

void Request::toLower(const std::string& src, std::string& out) const {
    out.reserve(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] >= 'A' && src[i] <= 'Z')
            out.push_back(src[i] + 32);
        else
            out.push_back(src[i]);
    }
}

bool Request::parseKeyValues(void) {
    size_t      spacer;
    std::string key;

    while (splitHeader("\r\n")) {
        if (valueBuffer.empty())
            return true;
        spacer = valueBuffer.find(':');
        if (spacer == std::string::npos)
            throw Router::ErrorException(400, "parseKeyValues: missing ':'");
        toLower(valueBuffer.substr(0, spacer), key);
        if (key.empty())
            throw Router::ErrorException(400, "parseKeyValues: empty header key");
        if (values.find(key) == values.end())
            values[key] = valueBuffer.substr(spacer + 2, -1);
        else
            values[key] += ", " + valueBuffer.substr(spacer + 2, -1);
        valueBuffer.clear();
        key.clear();
    }
    return false;
}

void Request::parseHeader(void) {
    if (!parseRequestLine())
        return;
    if (!parseKeyValues())
        return;
    haveHeader = true;
}

// #include <sys/time.h>
// static void timeStamp(const std::string& str) {
//     timeval currentTime;
//     gettimeofday(&currentTime, NULL);
//     long milliseconds = currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
//     std::cout << str << ": " << milliseconds << std::endl;
// }

void Request::parseBody(void) {
    std::map<std::string, std::string>::const_iterator it = values.find("transfer-encoding");
    if (it != values.end() && it->second == "chunked") {
        while (readPos != requestStr.size() && !haveBody) {
            skipCRLF();
            parseChunkedBody();
        }
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
    for (; readPos < requestStr.size(); ++readPos) {
        if (requestStr[readPos] != '\r' && requestStr[readPos] != '\n')
            return;
    }
}

bool Request::parseChunkSize(void) {
    size_t  hexDigit;
    bool    find = false;

    for (; readPos < requestStr.size(); ++readPos) {
        if (!find && (requestStr[readPos] == '\r' || requestStr[readPos] == '\n'))
            continue;
        hexDigit = hexToDecimal(requestStr[readPos]);
        if (hexDigit == static_cast<size_t>(-1)) {
            if (find) {
                return true;
            }
            throw Router::ErrorException(400, "parseChunkSize: invalid chunk size");
        }
        find = true;
        chunkSize =  chunkSize * 16 + hexDigit;
    }
    if (find) {
        return true;
    }
    return false;
}

void Request::parseChunkedBody(void) {
    size_t                              copySize;
    size_t                              bodySize;
    std::vector<char>::const_iterator   start;
    std::vector<char>::const_iterator   end;

    if (chunkSize == 0) {
        if (!parseChunkSize()) {
            return;
        }
        skipCRLF();
    }
    if (chunkSize == 0) {
        haveBody = true;
        return;
    }
    valueStart = readPos;
    bodySize = requestStr.size() - valueStart;
    if (bodySize < chunkSize) {
        copySize = bodySize;
        readPos += bodySize;
        chunkSize -= bodySize;
    }
    else {
        copySize = chunkSize;
        readPos += chunkSize;
        chunkSize = 0;
    }
    start = requestStr.begin() + valueStart;
    end = start + copySize;
    valueStart += copySize; //WIP
    while (start != end && *start != '\r') {
        body.push_back(*start);
        ++start;
    }
std::cout << "bodySize: " << body.size() << std::endl;
    skipCRLF();
}

void Request::parse() {
    if (!haveHeader) {
        parseHeader();
    }
    if (haveHeader) {
        parseBody();
    }
}
