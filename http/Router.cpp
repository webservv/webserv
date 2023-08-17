#include "Router.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>

std::map<std::string, std::string> Router::mimeMap;

void Router::initializeMimeMap() {
    if (mimeMap.empty()) {
        mimeMap["html"] = "text/html";
        mimeMap["txt"] = "text/plain";
        mimeMap["css"] = "text/css";
        mimeMap["js"] = "application/javascript";
        mimeMap["json"] = "application/json";
        mimeMap["xml"] = "application/xml";
        mimeMap["pdf"] = "application/pdf";
        mimeMap["zip"] = "application/zip";
        mimeMap["tar"] = "application/x-tar";
        mimeMap["gif"] = "image/gif";
        mimeMap["png"] = "image/png";
        mimeMap["jpg"] = "image/jpeg";
    }
}

Router::Router(const std::string& requestStr, const int clientSock)
    : request(requestStr), clientSocket(clientSock) {
    initializeMimeMap();
}

Router::~Router() {}

Router::Router(const Router& copy) {
	static_cast<void>(copy);
}

Router& Router::operator=(const Router& copy) {
	static_cast<void>(copy);
	return *this;
}

void Router::handleRequest() {

	if (request.getMethod() == Request::GET) {
		handleGet();
	} else if (request.getMethod() == Request::POST) {
		// handle POST
	} else if (request.getMethod() == Request::DELETE) {
		// handle DELETE
	} else
		return ;
}

void Router::sendResponse(const std::string& responseStr) {
    const char* responseBuffer = responseStr.c_str();
    size_t bytesToSend = responseStr.size();
    ssize_t bytesSent = 0;

    while (bytesToSend > 0) {
        ssize_t result = send(clientSocket, responseBuffer + bytesSent, bytesToSend, 0);
        if (result < 0) {
            std::cerr << "Error sending response to client: " << strerror(errno) << std::endl;
            break;
        }
        bytesSent += result;
        bytesToSend -= result;
    }
}

void Router::handleGet() {
    try {
        std::string filePath;
        parseURL(filePath);

        if (!resourceExists(filePath)) {
            response.makeStatusLine("HTTP/1.1", "404", "Not Found");
            return;
        }

        std::string content;
        readFile(filePath, content);
        std::string mimeType = getMIME(filePath);

        response.makeStatusLine("HTTP/1.1", "200", "OK");
        response.makeBody(content, content.size(), mimeType);

        sendResponse(response.getResponseStr());
    } catch (const std::ios_base::failure& e) {
        response.makeStatusLine("HTTP/1.1", "500", "Internal Server Error");
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

const std::string& Router::getResponseStr(void) const {
	return response.getResponseStr();
}
