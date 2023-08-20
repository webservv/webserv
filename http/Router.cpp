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
    Request::METHOD method = request.getMethod();

	if (method == Request::GET) {
        handleGet();
	} else if (method == Request::POST) {
		handlePost();
	} else if (method == Request::DELETE) {
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
	} catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    } 
}

void Router::handleDelete() {
	try {
		makeErrorResponse(501);
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