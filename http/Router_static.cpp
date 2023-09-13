#include "Router.hpp"
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <unistd.h>

void Router::processStaticGet(void) {
	const std::string	filePath = '.' + configURL;
	std::vector<char>	content;

	if (access(filePath.c_str(), F_OK)) {
		makeErrorResponse(404);
		return;
	}
    readFile(filePath, content);
    const std::string& mimeType = getMIME(configURL);
    response.makeStatusLine("HTTP/1.1", "200", "OK");
    response.makeBody(content, content.size(), mimeType);
	haveResponse = true;
}

void Router::processStaticPost(void) {
	const std::string			filePath = '.' + configURL;
	const std::vector<char>&	body = request.getBody();
	std::ofstream				os;

	if (body.empty()) {
		response.makeStatusLine("HTTP/1.1", "204", "No Content");
		response.endResponse();
		haveResponse = true;
		return;
	}
	os.open(filePath.c_str(), std::ios_base::app);
	if (os.fail())
		throw Router::ErrorException(500, "processStaticPost: open failure");
	os.write(body.data(), body.size());
	if (os.fail())
		throw Router::ErrorException(500, "processStaticPost: write failure");
	response.makeStatusLine("HTTP/1.1", "200", "OK");
	response.endResponse();
	haveResponse = true;
}

void Router::processStaticPut(void) {
	const std::string			filePath = '.' + configURL;
	const std::vector<char>&	body = request.getBody();
	std::ofstream				os;

	if (body.empty()) {
		response.makeStatusLine("HTTP/1.1", "204", "No Content");
		response.makeHeader("Content-Length", "0");
	}
	os.open(filePath, std::ios::out);
	if (os.fail())
		throw Router::ErrorException(500, "processStaticPut: open failure");
	os.write(body.data(), body.size());
	if (os.fail())
		throw Router::ErrorException(500, "processStaticPut: write failure");
	os.close();
	response.makeStatusLine("HTTP/1.1", "200", "OK");
	response.endResponse();
	haveResponse = true;
}

void Router::processStaticDelete(void) {
	const std::string& filePath = '.' + configURL;

	if (std::remove(filePath.c_str()) != 0) {
		throw Router::ErrorException(500, "processStaticDelete: remove system call error");
	}
	response.makeStatusLine("HTTP/1.1", "200", "OK");
	response.endResponse();
	haveResponse = true;
}