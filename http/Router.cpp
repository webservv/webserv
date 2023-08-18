#include "Router.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>

std::map<std::string, std::string> Router::mimeMap;
static const std::string    FAVICON_PATH = "./favicon/favicon.ico";

Router::Router():
	request(),
	response(),
	haveResponse(false) {
		initializeMimeMap();
	}

Router::~Router() {}

Router::Router(const Router& copy):
	request(copy.request),
	response(copy.response),
	haveResponse(copy.haveResponse) {
		initializeMimeMap();
	}

Router& Router::operator=(const Router& copy) {
	request = copy.request;
	response = copy.response;
	haveResponse = copy.haveResponse;
	return *this;
}

void Router::handleRequest() {

	if (request.getMethod() == Request::GET) {
		handleGet();
	} else if (request.getMethod() == Request::POST) {
		handlePost();
	} else if (request.getMethod() == Request::DELETE) {
		handleDelete();
	}
	haveResponse = true;
}

void Router::handleGet() {
	try {
		std::string filePath;
		parseURL(filePath);
		if (filePath == "./favicon.ico") {
			filePath = FAVICON_PATH;
		}
		if (!resourceExists(filePath)) {
			makeErrorPage();
			return;
		}

        std::string content;
        readFile(filePath, content);
        std::string mimeType = getMIME(filePath);

        response.makeStatusLine("HTTP/1.1", "200", "OK");
        response.makeBody(content, content.size(), mimeType);

	} catch (const std::ios_base::failure& e) {
		response.makeStatusLine("HTTP/1.1", "500", "Internal Server Error");
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

void Router::handlePost() {
	try {
		validateContentType();
		std::string title, postContent;
		parsePostData(title, postContent);
		appendPostToFile(title, postContent);
		std::string htmlResponse;
		readAndModifyHTML(htmlResponse);
		makeHTMLResponse(htmlResponse);
	} catch (const std::exception& e) {
		response.makeStatusLine("HTTP/1.1", "500", "Internal Server Error");
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

void Router::handleDelete() {
	try {
		response.makeStatusLine("HTTP/1.1", "501", "Not Implemented");
		response.makeBody("Delete method not implemented.", 30, "text/plain");
	} catch (const std::exception& e) {
		response.makeStatusLine("HTTP/1.1", "500", "Internal Server Error");
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

void Router::setResponse(const std::string &src) {
	response.setResponse(src);
}