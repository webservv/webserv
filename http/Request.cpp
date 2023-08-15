
#include "Request.hpp"
#include <iostream>

Request::Request( std::string request ) 
:requestParser(request) {
	std::string line;
	while (std::getline(requestParser, line)) {
		if (line.back() == '\r')
			line.pop_back();
		requestLines.push(line);
	}
	parseRequstLine();
	parseHeaders();
	parseBody();
}

// 'GET /index.html HTTP/1.1' <- example
void Request::parseRequstLine() {

	std::string& line = requestLines.front();
	requestLines.pop();

	size_t space = line.find(' ');
	if (space == std::string::npos)
		throw std::out_of_range("invalid http, request line!");
	std::string methodString = line.substr(0, space);
	line = line.substr(space + 1, -1);
	
	if (methodString == "GET")
		method = GET;
	else if (methodString == "POST")
		method = POST;
	else if (methodString == "DELETE")
		method = DELETE;
	else
		throw std::out_of_range("invalid http, request line!");

	space = line.find(' ', space + 1);
	if (space == std::string::npos)
		throw std::out_of_range("invalid http, request line!");
	url = line.substr(0, space);

	version = line.substr(space + 1, -1);
	if (version != "HTTP/1.1")
		throw std::out_of_range("invalid http, request line!");
	
	std::cout << "method: " << method << std::endl;
	std::cout << "url: " << url << std::endl;
	std::cout << "version: " << version << std::endl;
	
}

void Request::parseHeaders(void) {
	while (!requestLines.empty()) {
		std::string& line = requestLines.front();
		requestLines.pop();
		if (line.size() == 0)
			break;
		size_t index = line.find(':');
		if (index == std::string::npos)
			throw std::out_of_range("invalid http, header!");
		headers[line.substr(0, index)] = line.substr(index + 1, -1);
	}
	//test
	for (std::unordered_map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++) {
		std::cout << it->first << ": " << it->second << std::endl;
	}
}

void Request::parseBody(void) {
	while (!requestLines.empty()) {
		body += requestLines.front() + '\n';
		requestLines.pop();
	}
}