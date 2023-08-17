#include "Router.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>


static const std::string	g_dir = "./document";
static const std::string    g_error_dir = g_dir + "/error.html";
std::map<std::string, std::string> Router::mimeMap;
const std::string FAVICON_PATH = "webserv/favicon/favicon.ico";

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

const std::string& Router::getResponseStr(void) const {
	return response.getResponseStr();
}

void Router::sendErrorPage(void) {
    response.makeStatusLine("HTTP/1.1", "404", "Not Found");
    if (resourceExists(g_error_dir)) {
        std::string data;
        readFile(g_error_dir, data);
        response.makeBody(data, data.length(), getMIME(g_error_dir));
    }
}