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

Router::Router(std::string requestStr)
    : request(requestStr), response() {
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
    // return findMimeType(extension);
    return ("text/html");
}

void Router::handleGet() {

	std::string mapUrl = g_dir + request.getUrl();
	if (access( mapUrl.c_str(), F_OK ))
		response.makeStatusLine("HTTP/1.1", "404", "Not Found");
	else {
		response.makeStatusLine("HTTP/1.1", "200", "OK");
		std::ifstream ifs;
		ifs.open(mapUrl.c_str());
		if (!ifs.is_open())
			throw std::ios_base::failure("File open error. Router::handleget");
		std::string line;
		std::string data;
		while (std::getline(ifs, line)) {
			data += line + "\r\n";
		}
		response.makeBody(data, data.size(), getMIME(request.getUrl()));
	// 4. Determine the Content Type
	// 5. Send the Response Header
	}
	//test
	// std::cout << response.getResponseStr() << std::endl;
}

const std::string& Router::getResponseStr(void) const {
	return response.getResponseStr();
}
