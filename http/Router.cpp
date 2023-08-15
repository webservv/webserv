#include "Router.hpp"
#include "Request.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>

static const std::string	g_dir = "./document";

Router::Router(std::string requestStr) 
:request(requestStr), response() {
	
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

std::string Router::getMIME(std::string url) {

	(void)(url);
	return ("text'html");
}

void Router::handleGet() {

	std::string mapUrl = g_dir + request.getUrl();
	if (!access( mapUrl.c_str(), F_OK ))
		response.makeStatusLine("HTTP/1.1", "404", "Not Found");
	else {
		response.makeStatusLine("HTTP/1.1", "200", "OK");
		std::ifstream ifs;
		ifs.open(mapUrl.c_str());
		if (!ifs.is_open())
			throw ;//
		std::string line;
		std::string data;
		while (std::getline(ifs, line)) {
			data += line;
		}
		response.makeBody(data, data.size(), getMIME(request.getUrl()));
	// 4. Determine the Content Type
	// 5. Send the Response Header
	}
	//test
	std::cout << response.getResponseStr() << std::endl;
}
