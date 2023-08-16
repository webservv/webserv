#include "Router.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>

static const std::string	g_dir = "./document";
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

Router::Router(std::string requestStr, int clientSock)
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

std::string Router::getExtension(const std::string& url) {
    size_t extensionStart = url.find_last_of('.');
    if (extensionStart == std::string::npos) {
        return "";
    }
    return url.substr(extensionStart + 1);
}

std::string Router::findMimeType(const std::string& extension) {
    std::map<std::string, std::string>::const_iterator it = mimeMap.find(extension);
    if (it != mimeMap.end()) {
        return it->second;
    } else {
        return "application/octet-stream";
    }
}

std::string Router::getMIME(std::string url) {
    std::string extension = getExtension(url);
    return findMimeType(extension);
}

void Router::parseURL(std::string& filePath) {
    std::string urlPath = request.getUrl();
    filePath = g_dir + urlPath;
}

bool Router::resourceExists(const std::string& filePath) {
    return !access(filePath.c_str(), F_OK);
}

void Router::readFile(const std::string& filePath, std::string& content) {
    std::ifstream ifs(filePath.c_str());
    if (!ifs.is_open()) {
        throw std::ios_base::failure("File open error.");
    }
    content.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
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
        response.makeHeader("Content-Type", mimeType);
        response.makeHeader("Content-Length", std::to_string(content.size()));
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
