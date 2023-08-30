#include "Router.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"

#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>

std::map<std::string, std::string> Router::mimeMap;
static const std::string    FAVICON_PATH = "./favicon/favicon.ico";
static int cookieId = 0;

Router::Router():
	request(),
	response(),
	haveResponse(false),
	server(NULL),
	clientAddr(),
	config(NULL),
	CgiVariables(),
	configURL(){
		initializeMimeMap();
	}

Router::Router(Server* const server, const sockaddr_in& clientAddr, const Config::server* config):
	request(),
	response(),
	haveResponse(false),
	server(server),
	clientAddr(clientAddr),
	config(config),
	CgiVariables(),
	configURL(){
		initializeMimeMap();
	}


Router::Router(const Router& copy):
	request(copy.request),
	response(copy.response),
	haveResponse(copy.haveResponse),
	server(copy.server),
	clientAddr(copy.clientAddr),
	config(copy.config),
	CgiVariables(copy.CgiVariables),
	configURL(copy.configURL) {
		initializeMimeMap();
	}

Router& Router::operator=(const Router& copy) {
	request = copy.request;
	response = copy.response;
	haveResponse = copy.haveResponse;
	server = copy.server;
	clientAddr = copy.clientAddr;
	config = copy.config;
	CgiVariables = copy.CgiVariables;
	configURL = copy.configURL;
	return *this;
}

Router::~Router() {}

void Router::handleGet() {
	const std::string& filePath = CgiVariables["SCRIPT_NAME"];

	try {
		if (!filePath.compare(0, 4, "/cgi")) {
			response.makeStatusLine("HTTP/1.1", "200", "OK");
			connectCGI();
		}
		else {
			// if (!resourceExists(filePath)) { -> delete one line function. only used here 
			if (access(filePath.c_str(), F_OK)) {
				makeErrorResponse(404);
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
	validateHeaderLength();
	validateContentType();
	response.makeStatusLine("HTTP/1.1", "201", "OK");
	connectCGI();
}

void Router::handleDelete() {
	response.makeStatusLine("HTTP/1.1", "200", "OK");
	connectCGI();
}

void Router::connectCGI(void) {
	makeCgiVariables();
    if (needCookie()) {
        const std::string& cookie = request.findValue("Cookie");
        if (cookie.empty()) {
            cookieId++;
            std::stringstream ss;
            ss << cookieId;
            std::string cookieIdStr = ss.str();
            server->addCookie(cookieIdStr, "SessionValues");
            response.makeHeader("Set-Cookie", "SessionID=" + cookieIdStr);
    	}
	}
	response.setMessageToCGI(request.getBody());
	response.connectCGI(CgiVariables);
	server->addPipes(response.getWriteFD(), response.getReadFD(), this);
}

bool Router::isBodyRequired(void) const {
    Request::METHOD method = request.getMethod();
    switch (method) {
        case Request::POST:
            return true;
        default:
            return false;
    }
}

const std::string& Router::getResponseStr(void) const {
	return response.getResponseStr();
}

const std::string& Router::getParsedURL(void) const {
    return configURL;
}

void Router::handleRequest() {
    Request::METHOD method = request.getMethod();
	
	setParsedURL();
	parseURL();
	if (method == Request::GET) {
        handleGet();
	} else if (method == Request::POST) {
		handlePost();
	} else if (method == Request::DELETE) {
		handleDelete();
	}
}

const Config::server* Router::getConfig(void) const {
	return config;
}

const sockaddr_in& Router::getClientAddr(void) const {
	return clientAddr;
}

bool Router::isHeaderEnd(void) const {
	return request.isHeaderEnd();
}

bool Router::isRequestEnd() const {
	return request.isRequestEnd();
}

void Router::parseRequest(void) {
    request.parse();
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

void Router::setResponse(const std::string &src) {
	response.setResponse(src);
}

void Router::readFromCGI(void) {
	response.readFromCGI();
}

void Router::writeToCGI(const intptr_t fdBufferSize) {
	response.writeToCGI(fdBufferSize);
}

void Router::disconnectCGI(void) {
	response.disconnectCGI();
	haveResponse = true;
}

int Router::getWriteFD(void) const {
	return response.getWriteFD();
}

int Router::getReadFD(void) const {
	return response.getReadFD();
}

int Router::getRequestError() const {
    return request.getError();
}