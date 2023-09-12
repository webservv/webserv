#include "Router.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"

#include <exception>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

static const std::string g_methodStr[] = {
    "GET",
    "POST",
    "DELETE",
    "OTHER"
};

std::map<std::string, std::string> Router::mimeMap;
static const std::string    FAVICON_PATH = "./favicon/favicon.ico";
static int cookieId = 0;

Router::ErrorException::ErrorException(const int errorCode, const std::string& message)
	: errorCode(errorCode)
	, message(message) {}

Router::ErrorException::~ErrorException() throw() {}

const char* Router::ErrorException::what(void) const throw() {
	return message.c_str();
}

int Router::ErrorException::getErrorCode(void) const {
	return errorCode;
}

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

Router::Router(Server* const server, const sockaddr_in& clientAddr, const ServerConfig* config):
	request(),
	response(),
	haveResponse(false),
	server(server),
	clientAddr(clientAddr),
	config(config),
	CgiVariables(),
	configURL(), 
	matchLocation(NULL) {
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
	configURL(copy.configURL),
	matchLocation(NULL) {
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
	matchLocation = copy.matchLocation;
	return *this;
}

Router::~Router() {}

void Router::handleGet() {
	if (configURL.find("/cgi/") != std::string::npos) {
        connectCGI();
	}
	else
		processStaticGet();
}

void Router::handlePost() {
	if (isInvalidBodySize())
		throw Router::ErrorException(413, "handlePost: body size is too large");
	validateHeaderLength();
	validateContentType();
	if (configURL.find("/cgi/") != std::string::npos) {
		connectCGI();
	}
	else
		processStaticPost();
}

void Router::handleDelete() {
	if (configURL.find("/cgi/") != std::string::npos) {
		connectCGI();
	}
	else {
		processStaticDelete();
	}
}

void Router::handlePut(void) {
	if (isInvalidBodySize())
		throw Router::ErrorException(413, "handlePost: body size is too large");
	processStaticPut();
}

void Router::connectCGI(void) {
	response.makeStatusLine("HTTP/1.1", "200", "OK");
	parseURL();
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
    const std::string& method = request.getMethod();
	if (method == "POST" || method == "PUT")
		return true;
	return false;
}

const std::string& Router::getParsedURL(void) const {
    return configURL;
}

void Router::handleMethod(const std::string& method) {
    const std::vector<std::string>&	limit = matchLocation->getLimitExcept();

    if (limit.empty())
        return ;
    std::vector<std::string>::const_iterator it = std::find(limit.begin(), limit.end(), method);
    if (it == limit.end()) {
        throw Router::ErrorException(405, "Method Not Allowed");
    }
}

void Router::handleRedirect(const std::string& url) {
    response.makeStatusLine("HTTP/1.1", "301", "Moved Permanently");
    response.makeHeader("Location", url);
    haveResponse = true;
}


void Router::handleRequest() {
    const std::string& method = request.getMethod();
	try {
		setConfigURL();
        if (!matchLocation->getReturnURL().empty()) {
            handleRedirect(matchLocation->getReturnURL());
            return ;
        }
    	handleMethod(method);
		if (method == "GET") {
    	    handleGet();
		} else if (method == "POST") {
			handlePost();
		} else if (method == "DELETE") {
			handleDelete();
		} else if (method == "PUT")
			handlePut();
	}
	catch (Router::ErrorException& e) {
		std::cout << e.what() << std::endl;
		makeErrorResponse(e.getErrorCode());
	}
}

const ServerConfig* Router::getConfig(void) const {
	return config;
}

const sockaddr_in& Router::getClientAddr(void) const {
	return clientAddr;
}

bool Router::isRequestEnd() const {
	return request.isRequestEnd();
}

bool Router::getHaveResponse(void) const {
	return haveResponse;
}

const std::vector<char>& Router::getRequest(void) const {
	return request.getRequestStr();
}

const std::vector<char>& Router::getResponse(void) const {
	return response.getResponse();
}

size_t Router::getSentLength(void) const {
	return response.getSentLength();
}

void Router::setSentLength(const size_t size) {
	response.setSentLength(size);
}

// #include <sys/time.h>
// static void timeStamp(const std::string& str) {
//     timeval currentTime;
//     gettimeofday(&currentTime, NULL);
//     long milliseconds = currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
//     std::cout << str << ": " << milliseconds << std::endl;
// }

void Router::addRequest(const Buffer& input) {
	try {
		request.addRequest(input);
    	request.parse();
	}
	catch (Router::ErrorException& e) {
		std::cerr << e.what() << std::endl;
		makeErrorResponse(e.getErrorCode());
	}
}

void Router::setResponse(const std::vector<char>& src) {
	response.setResponse(src);
}

void Router::readFromCGI(void) {
	try {
		response.readFromCGI();
	}
	catch (Router::ErrorException& e) {
		std::cerr << e.what() << std::endl;
		makeErrorResponse(e.getErrorCode());
	}
}

void Router::writeToCGI(const intptr_t fdBufferSize) {
	try {
		response.writeToCGI(fdBufferSize);
	}
	catch (Router::ErrorException& e) {
		std::cerr << e.what() << std::endl;
		makeErrorResponse(e.getErrorCode());
	}
}

void Router::disconnectCGI(void) {
	try {
		response.disconnectCGI();
	}
	catch (Router::ErrorException& e) {
		std::cerr << e.what() << std::endl;
		makeErrorResponse(e.getErrorCode());
	}
	haveResponse = true;
}

int Router::getWriteFD(void) const {
	return response.getWriteFD();
}

int Router::getReadFD(void) const {
	return response.getReadFD();
}