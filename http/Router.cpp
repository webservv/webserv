#include "Router.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"

#include <unistd.h>
#include <fstream>
#include <iostream>

std::map<std::string, std::string> Router::mimeMap;
static const std::string    FAVICON_PATH = "./favicon/favicon.ico";

Router::Router():
	request(),
	response(),
	haveResponse(false),
	server(NULL) {
		initializeMimeMap();
	}

Router::Router(Server* const server):
	request(),
	response(),
	haveResponse(false),
	server(server) {
		initializeMimeMap();
	}

Router::~Router() {}

Router::Router(const Router& copy):
	request(copy.request),
	response(copy.response),
	haveResponse(copy.haveResponse),
	server(copy.server) {
		initializeMimeMap();
	}

Router& Router::operator=(const Router& copy) {
	request = copy.request;
	response = copy.response;
	haveResponse = copy.haveResponse;
	server = copy.server;
	return *this;
}

void Router::handleRequest() {
    Request::METHOD method = request.getMethod();

	if (method == Request::GET) {
        handleGet();
	} else if (method == Request::POST) {
		handlePost();
	} else if (method == Request::DELETE) {
		handleDelete();
	}
}

void Router::handleGet() {
	try {
		std::string filePath;
		parseURL(filePath);
		if (filePath == "./favicon.ico") {
			filePath = FAVICON_PATH;
		}
		if (!filePath.compare(0, 4, "/cgi")) {
			response.makeStatusLine("HTTP/1.1", "200", "OK");
			std::map<std::string, std::string> envs;
			envs["SCRIPT_NAME"] = "./cgi/index.py";
			envs["SERVER_NAME"] = "default server";
			envs["id"] = "testID";
			response.connectCGI(envs);
			server->addPipes(response.getWriteFd(), response.getReadFd(), this);
		}
		else {
			if (!resourceExists(filePath)) {
				makeErrorPage();
				return;
			}
			std::string content;
        	readFile(filePath, content);
        	std::string mimeType = getMIME(filePath);
        	response.makeStatusLine("HTTP/1.1", "200", "OK");
        	response.makeBody(content, content.size(), mimeType);
			haveResponse = true;
		}
        

	} catch (const std::ios_base::failure& e) {
        makeErrorResponse(500);
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

void Router::handlePost() {
	try {
        validateHeaderLength();
		validateContentType();
		std::string title, postContent;
		parsePostData(title, postContent);
		appendPostToFile(title, postContent);
		std::string htmlResponse;
		readAndModifyHTML(htmlResponse);
		makeHTMLResponse(htmlResponse);
		haveResponse = true;
	} catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    } 
}

void Router::handleDelete() {
	try {
		response.makeStatusLine("HTTP/1.1", "501", "Not Implemented");
		response.makeBody("Delete method not implemented.", 30, "text/plain");
		haveResponse = true;
	} catch (const std::exception& e) {
		makeErrorResponse(500);
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

bool Router::isHeaderEnd() {
	return request.isHeaderEnd();
}

bool Router::isRequestEnd() {
	return request.isRequestEnd();
}

bool Router::getHaveResponse(void) const {
	return haveResponse;
}

const std::string& Router::getResponse(void) const {
	return response.getResponseStr();
}

void Router::addRequest(const std::string &request) {
	this->request.addRequest(request);
}

void Router::parseHeader(void) {
	request.parseHeader();
}

void Router::parseBody(void) {
	request.parseBody();
}

void Router::setResponse(const std::string &src) {
	response.setResponse(src);
}

void Router::readCGI(void) {
	response.readCGI();
}

void Router::writeCGI(const intptr_t fdBufferSize) {
	response.writeCGI(fdBufferSize);
}

void Router::disconnectCGI(void) {
	response.disconnectCGI();
	haveResponse = true;
}

int Router::getWriteFd(void) const {
	return response.getWriteFd();
}

int Router::getReadFd(void) const {
	return response.getReadFd();
}