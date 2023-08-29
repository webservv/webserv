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
	config(NULL) {
		initializeMimeMap();
	}

Router::Router(Server* const server, const sockaddr_in& clientAddr, const Config::server* config):
	request(),
	response(),
	haveResponse(false),
	server(server),
	clientAddr(clientAddr),
	config(config) {
		initializeMimeMap();
	}

Router::~Router() {}

Router::Router(const Router& copy):
	request(copy.request),
	response(copy.response),
	haveResponse(copy.haveResponse),
	server(copy.server),
	clientAddr(copy.clientAddr),
	config(copy.config) {
		initializeMimeMap();
	}

Router& Router::operator=(const Router& copy) {
	request = copy.request;
	response = copy.response;
	haveResponse = copy.haveResponse;
	server = copy.server;
	clientAddr = copy.clientAddr;
	config = copy.config;
	return *this;
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

void Router::handleGet() {
	const std::string& filePath = CgiVariables["SCRIPT_NAME"];

	try {
		if (!filePath.compare(0, 4, "/cgi"))
			connectCGI();
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
	validateHeaderLength();
	validateContentType();
	connectCGI();
}

void Router::handleDelete() {
	connectCGI();
}

void Router::connectCGI(void) {
	response.makeStatusLine("HTTP/1.1", "200", "OK");
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

const sockaddr_in& Router::getClientAddr(void) const {
	return clientAddr;
}

const Config::server* Router::getConfig(void) const {
	return config;
}