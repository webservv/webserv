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
    const std::string&  url = line.substr(0, space);
    const size_t        pathIndex = url.find('/', 5); // hard coding. only /cgi/* can be parsed.
    const size_t        queryIndex = url.find('?');
    
    values["script_name"] = url.substr(0, pathIndex);
    if (pathIndex != std::string::npos)
        values["path_info"] = url.substr(pathIndex, queryIndex - pathIndex);
    if (queryIndex != std::string::npos)
        values["query_string"] = url.substr(queryIndex + 1, -1);
}

void Request::parseVersion(const std::string& line, const size_t space) {
    values["version"] = line.substr(space + 1);
    if (values["version"] != "HTTP/1.1") {
        error = 505;
        throw std::out_of_range("invalid http, request line! Unsupported version: " + values["version"]);
    }
}

void Request::parseBody(void) {
    while (!requestLines.empty()) {
        values["body"] += requestLines.front();
        requestLines.pop();
    }
}

void Request::addRequestLines(void) {
    std::stringstream parser(requestStr);
    bool isChunked = false;

    if (values.find("transfer-encoding") != values.end() && values["transfer-encoding"] == "chunked")
        isChunked = true;
    readHeadersAndInitialRequestLines(parser);
    if (isChunked) {
        handleChunkedTransferEncoding(parser);
    } else {
        handleNonChunkedTransferEncoding(parser);
    }
std::cout << requestStr << std::endl;
    requestStr.clear();
}

void Request::readHeadersAndInitialRequestLines(std::stringstream& parser) {
    std::string line;
    while (std::getline(parser, line) && !line.empty()) {
        if (line.back() == '\r') line.pop_back();
        requestLines.push(line);
    }
}

void Request::handleNonChunkedTransferEncoding(std::stringstream& parser) {
    std::string line;
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
            error = 413;
            throw std::out_of_range("invalid http, chunk size is too big!");
        }

        std::vector<char> buffer(chunkSize);
        parser.read(buffer.data(), chunkSize);
        std::string chunkData(buffer.begin(), buffer.end());
        requestLines.push(chunkData);
        parser.ignore(2); // Ignore the \r\n after the chunk
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

    parseRequestLine();
    parseKeyValues();
    haveHeader = true;
}

void Request::parse(void) {
    addRequestLines();
    parseHeader();
    parseBody();
}
