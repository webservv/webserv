#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/_types/_size_t.h>
#include <vector>
#include "Router.hpp"

const size_t MAX_CHUNK_SIZE = 1024 * 1024; // 1 MB
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

void Request::parseBody(void) {
    std::map<std::string, std::string>::const_iterator it = values.find("transfer-encoding");
    
    if (it != values.end() && it->second == "chunked")
        parseChunkedBody();
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

void Request::parseChunkedBody(void) {
    std::stringstream   parser;
    std::string         line;
    size_t              chunkSize;

    if (!isChunkEnd())
        return;
    for (size_t i = bodyPos; i < requestStr.size(); ++i) {
        parser << requestStr[i];
    }
    while (std::getline(parser, line)) {
        std::stringstream chunkSizeStream(line);
        
        if (line == "\r")
            continue;
        chunkSizeStream >> std::hex >> chunkSize;
        if (chunkSizeStream.fail() || chunkSize == 0) {
            haveBody = !chunkSize;
            break;
        }
        if (chunkSize > MAX_CHUNK_SIZE) {
            throw Router::ErrorException(413, "invalid http, chunk size is too big!");
        }
        std::vector<char> buffer(chunkSize);
        parser.read(buffer.data(), chunkSize);
        body.insert(body.end(), buffer.begin(), buffer.end());        
    }
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
