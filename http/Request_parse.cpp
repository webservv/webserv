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

void Request::parseMethod(std::string& line) {
    size_t space = line.find(' ');
    if (space == std::string::npos) {
        throw Router::ErrorException(400, "invalid http, request line! Missing or misplaced space");
    }
    std::string methodString = line.substr(0, space);
    line = line.substr(space + 1);
    if (methodString == "GET") {
        method = GET;
    }
    else if (methodString == "POST") {
        method = POST;
    }
    else if (methodString == "DELETE") {
        method = DELETE;
    }
    else if (methodString == "PUT") {
        method = PUT;
    }
    else {
        method = OTHER;
        throw Router::ErrorException(405, "invalid http, request line! Unsupported method: " + methodString);
    }
    values["method"] = methodString;
}

void Request::parseURL(const std::string& line) {
    const size_t space = line.find(' ');
    if (space == std::string::npos) {
        throw Router::ErrorException(400, "invalid http, request line! Missing or misplaced space");
    }
    values["url"] = line.substr(0, space);
}

void Request::parseVersion(const std::string& line, const size_t space) {
    values["version"] = line.substr(space + 1);
    if (values["version"] != "HTTP/1.1") {
        throw Router::ErrorException(505, "invalid http, request line! Unsupported version: " + values["version"]);
    }
}

// static void timeStamp(int i) {
//     std::time_t Time = std::time(NULL);
//     std::string timeStr = std::ctime(&Time);
//     std::cout << "Time" << i << " : " << timeStr << std::endl;
// }

void Request::parseBody(void) {
    std::map<std::string, std::string>::const_iterator it = values.find("transfer-encoding");
// timeStamp(1);
    if (it != values.end() && it->second == "chunked") {
        while (bodyPos != requestStr.size())
            parseChunkedBody();
        }
    else {
        std::vector<char>::iterator st = requestStr.begin() + bodyPos;
        if (st < requestStr.end()) {
            body.insert(body.end(), st, requestStr.end());
            bodyPos = requestStr.size();
        }
        it = values.find("content-length");
        if (it == values.end()) {
            haveBody = true;
            return;
        }
        size_t len = std::atoi(it->second.c_str());
        if (body.size() == len)
            haveBody = true;
    }
// timeStamp(2);
}

void Request::addRequestLines(void) {
    std::stringstream   parser;
    std::string         line;

    for (size_t i = 0; i < bodyPos; ++i) {
        parser << requestStr[i];
    }
    while (std::getline(parser, line) && !line.empty()) {
        if (line.back() == '\r')
            line.pop_back();
        requestLines.push(line);
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
    if (requestStr.size() - bodyPos >= 2) {
        if (requestStr[bodyPos] == '\r' && requestStr[bodyPos + 1] == '\n')
            bodyPos += 2;
    }
}

bool Request::parseChunkSize(void) {
    size_t  hexDigit;

    for (; bodyPos < requestStr.size() - 1; ++bodyPos) {
        if ((requestStr[bodyPos] == '\r' && requestStr[bodyPos + 1] == '\n')) {
            bodyPos += 2;
            chunkStart = bodyPos;
            return true;
        }
        hexDigit = hexToDecimal(requestStr[bodyPos]);
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
    bodySize = requestStr.size() - chunkStart;
    if (bodySize < chunkSize) {
        copySize = bodySize;
        bodyPos = requestStr.size();
        chunkSize -= bodySize;
    }
    else {
        copySize = chunkSize;
        bodyPos += chunkSize;
        chunkSize = 0;
    }
    start = requestStr.begin() + chunkStart;
    end = start + copySize;
    body.insert(body.end(), start, end);
    skipCRLF();
}

void Request::parseRequestLine() {
    if (requestLines.empty()) {
        throw Router::ErrorException(400, "invalid http, empty request line!");
    }
    std::string line = requestLines.front();
    requestLines.pop();
    parseMethod(line);
    size_t space = line.find(' ');
    parseURL(line);
    parseVersion(line, space);
}

void Request::parseKeyValues(void) {
    std::string line;
    std::string headerName;
    size_t      index;

	while (!requestLines.empty()) {
		line = requestLines.front();
		requestLines.pop();
		if (line.size() == 0)
			break;
		index = line.find(": ");
		if (index == std::string::npos || index + 2 >= line.size()) {
			throw Router::ErrorException(400, "invalid http, header!");
        }
        headerName = line.substr(0, index);
        std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
        std::map<std::string, std::string>::iterator    it = values.find(headerName);
        if (it != values.end())
            it->second += ", " + line.substr(index + 2);
        else
	    	values[headerName] = line.substr(index + 2);
	}
}

void Request::parseHeader(void) {
    if (haveHeader)
        return;
    addRequestLines();
    parseRequestLine();
    parseKeyValues();
    haveHeader = true;
}

void Request::parse() {
    parseHeader();
    parseBody();
}
