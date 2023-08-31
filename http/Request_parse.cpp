#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/_types/_size_t.h>
#include <vector>

const size_t MAX_CHUNK_SIZE = 1024 * 1024; // 1 MB
static const std::string POST_URL = "/cgi/index.php";

void Request::parseMethod(std::string& line) {
    size_t space = line.find(' ');
    if (space == std::string::npos) {
        error = 400;
        throw std::out_of_range("invalid http, request line! Missing or misplaced space");
    }
    std::string methodString = line.substr(0, space);
    line = line.substr(space + 1);
    if (methodString == "GET") {
        method = GET;
        values["method"] = "GET";
    }
    else if (methodString == "POST") {
        method = POST;
        values["method"] = "POST";
    }
    else if (methodString == "DELETE") {
        method = DELETE;
        values["method"] = "DELETE";
    }
    else {
        error = 405;
        throw std::out_of_range("invalid http, request line! Unsupported method: " + methodString);
    }
}

void Request::parseURL(const std::string& line) {
    const size_t space = line.find(' ');
    if (space == std::string::npos) {
        error = 400;
        throw std::out_of_range("invalid http, request line! Missing or misplaced space");
    }
    values["url"] = line.substr(0, space);
}

void Request::parseVersion(const std::string& line, const size_t space) {
    values["version"] = line.substr(space + 1);
    if (values["version"] != "HTTP/1.1") {
        error = 505;
        throw std::out_of_range("invalid http, request line! Unsupported version: " + values["version"]);
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

    for (size_t i = bodyPos; i < requestStr.size(); ++i) {
        parser << requestStr[i];
    }
    while (std::getline(parser, line)) {
        line += '\n';
        std::stringstream chunkSizeStream(line);
        chunkSizeStream >> std::hex >> chunkSize;
        if (chunkSizeStream.fail() || chunkSize == 0) {
            break;
        }
        if (chunkSize > MAX_CHUNK_SIZE) {
            error = 413;
            throw std::out_of_range("invalid http, chunk size is too big!");
        }
        std::vector<char> buffer(chunkSize);
        parser.read(buffer.data(), chunkSize);
        std::string chunkData(buffer.begin(), buffer.end());
        values["body"] += line;
    }
}

void Request::parseRequestLine() {
    if (requestLines.empty()) {
        error = 400;
        throw std::out_of_range("invalid http, empty request line!");
    }
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
		if (index == std::string::npos || index + 2 >= line.size()) {
            error = 400;
			throw std::out_of_range("invalid http, header!");
        }
        std::string headerName = line.substr(0, index);
        std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);

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
    if (values.find("content-length") != values.end()) {
        std::stringstream   ss(values["content-length"]);
        size_t              size;
        ss >> size;
        values["body"].reserve(size);
    }
}

void Request::parse() {
    parseHeader();
    parseBody();
}
