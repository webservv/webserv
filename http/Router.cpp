#include "Router.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>

std::map<std::string, std::string> Router::mimeMap;
const std::string           FAVICON_PATH = "./favicon/favicon.ico";

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
		handlePost();
	} else if (request.getMethod() == Request::DELETE) {
		handleDelete();
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
		if (filePath == "./favicon.ico") {
			filePath = FAVICON_PATH;
		}
		if (!resourceExists(filePath)) {
			sendErrorPage();
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

void Router::handlePost() {
	try {
		std::string contentType = request.getHeaderValue("Content-Type");
		if (contentType != "application/x-www-form-urlencoded") {
			response.makeStatusLine("HTTP/1.1", "415", "Unsupported Media Type");
			return;
		}

		std::string content = request.getBody();
        std::stringstream ss(content);
        std::string key;
        std::string value;
        std::string body;
        while (std::getline(ss, key, '=')) {
            std::getline(ss, value, '&');
            body += key + " = " + value + '\n';
        }

		response.makeStatusLine("HTTP/1.1", "200", "OK");
		response.makeBody(body, body.size(), "text/plain");

		sendResponse(response.getResponseStr());
	} catch (const std::exception& e) {
		response.makeStatusLine("HTTP/1.1", "500", "Internal Server Error");
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

void Router::handleDelete() {
	try {
		response.makeStatusLine("HTTP/1.1", "501", "Not Implemented");
		response.makeBody("Delete method not implemented.", 30, "text/plain");
		sendResponse(response.getResponseStr());
	} catch (const std::exception& e) {
		response.makeStatusLine("HTTP/1.1", "500", "Internal Server Error");
		std::cerr << "Error: " << e.what() << std::endl;
	}
}
