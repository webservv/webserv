#include "Router.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"

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
	const std::string& filePath = request.getPath();
	std::map<std::string, std::string> envs;

	try {
		if (!filePath.compare(0, 5, "./cgi")) {
			response.makeStatusLine("HTTP/1.1", "200", "OK");
			connectCGI();
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
	validateHeaderLength();
	validateContentType();
	connectCGI();
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

void Router::connectCGI(void) {
	std::map<std::string, std::string> envs;

	makeCGIenvs(envs);
    if (request.isHaveCookie()) {
        const std::string& cookie = request.findValue("Cookie");
        if (cookie == "") {
            cookieId++;
            std::stringstream ss;
            ss << cookieId;
            std::string cookieIdStr = ss.str();
            server->addCookie(cookieIdStr, request.getBody());
            response.makeHeader("Set-Cookie", cookieIdStr);
        } else {
            // std::cout << "cookie: " << cookie << std::endl;
        }
    }
	response.setMessageToCGI(request.getBody());
	response.connectCGI(envs);
	server->addPipes(response.getWriteFd(), response.getReadFd(), this);
}