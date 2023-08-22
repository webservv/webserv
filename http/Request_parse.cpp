#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <sys/_types/_size_t.h>
#include <vector>

const size_t MAX_CHUNK_SIZE = 1024 * 1024; // 1 MB

void Request::parseMethod(std::string& line) {
    size_t space = line.find(' ');
    if (space == std::string::npos)
        throw std::out_of_range("invalid http, request line! Missing or misplaced space");
    std::string methodString = line.substr(0, space);
    line = line.substr(space + 1);
    if (methodString == "GET")
        method = GET;
    else if (methodString == "POST")
        method = POST;
    else if (methodString == "DELETE")
        method = DELETE;
    else
        throw std::out_of_range("invalid http, request line! Unsupported method: " + methodString);
}

void Request::parseURL(std::string& line) {
    size_t space = line.find(' ');
    if (space == std::string::npos)
        throw std::out_of_range("invalid http, request line! Missing or misplaced space");
    url = line.substr(0, space);
}

void Request::parseVersion(std::string& line, size_t space) {
    version = line.substr(space + 1);
    if (version != "HTTP/1.1")
        throw std::out_of_range("invalid http, request line! Unsupported version: " + version);
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

void Request::addRequestLines(void) {
    std::string line;
    std::stringstream parser(requestStr);
    parser.clear();
    parser.seekg(0);

    readHeadersAndInitialRequestLines(parser);
    bool isChunked = headers["transfer-encoding"] == "chunked";
    
    if (isChunked) {
        handleChunkedTransferEncoding(parser);
    } else {
        handleNonChunkedTransferEncoding(parser);
    }

    requestStr.clear();
}

void Request::readHeadersAndInitialRequestLines(std::stringstream& parser) {
    std::string line;
    while (std::getline(parser, line) && !line.empty()) {
        if (line.back() == '\r')
            line.pop_back();
        requestLines.push(line);
    }
}

void Request::handleNonChunkedTransferEncoding(std::stringstream& parser) {
    std::string line;
    if (requestStr.find("0\r\n") == 0) {
        throw std::out_of_range("non-chunked transfer encoding needs to have body!");
    }
    while (std::getline(parser, line)) {
        if (line.back() == '\r') line.pop_back();
        requestLines.push(line);
    }
}

void Request::handleChunkedTransferEncoding(std::stringstream& parser) {
    std::string line;
    while (true) {
        std::getline(parser, line);
        if (line.back() == '\r') line.pop_back();
        size_t chunkSize;
        std::stringstream chunkSizeStream(line);
        chunkSizeStream >> std::hex >> chunkSize;

        if (chunkSizeStream.fail() || chunkSize == 0) {
            break;
        }

        if (chunkSize > MAX_CHUNK_SIZE) {
            throw std::out_of_range("invalid http, chunk size!");
        }

        std::vector<char> buffer(chunkSize);
        parser.read(buffer.data(), chunkSize);
        std::string chunkData(buffer.begin(), buffer.end());
        requestLines.push(chunkData);
        parser.ignore(2);
    }
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
        std::string headerName = line.substr(0, index);
        std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
        
		headers[headerName] = line.substr(index + 2);
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
