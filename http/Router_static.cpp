#include "Router.hpp"
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>
#include <cstdio>

void Router::processStaticGet(void) {
	const std::string	filePath = '.' + configURL;

	if (!resourceExists(filePath.c_str())) {
		makeErrorResponse(404);
		return;
	}
	std::string content;
    readFile(filePath, content);
    std::string mimeType = getMIME(configURL);
    response.makeStatusLine("HTTP/1.1", "200", "OK");
    response.makeBody(content, content.size(), mimeType);
	haveResponse = true;
}

void Router::processStaticPost(void) {
	const std::vector<char>&	body = request.getBody();
	std::ofstream				os;

	if (body.empty()) {
		response.makeStatusLine("HTTP/1.1", "204", "No Content");
		response.makeHeader("Content-Length", "0");
	}
	os.open(configURL.c_str(), std::ios_base::app);
	if (os.fail())
		throw Router::ErrorException(500, "processStaticPost: open failure");
	os.write(body.data(), body.size());
	if (os.fail())
		throw Router::ErrorException(500, "processStaticPost: write failure");
	response.makeStatusLine("HTTP/1.1", "200", "OK");
	haveResponse = true;
}

void Router::processStaticPut(void) {
	const std::vector<char>&	body = request.getBody();
	std::ofstream				os;

	if (body.empty()) {
		response.makeStatusLine("HTTP/1.1", "204", "No Content");
		response.makeHeader("Content-Length", "0");
	}
	os.open(configURL.c_str(), std::ios::out);
	if (os.fail())
		throw Router::ErrorException(500, "processStaticPost: open failure");
	os.write(body.data(), body.size());
	if (os.fail())
		throw Router::ErrorException(500, "processStaticPost: write failure");
	response.makeStatusLine("HTTP/1.1", "200", "OK");
	haveResponse = true;
}

void Router::processStaticDelete(void) {
	if (!resourceExists(configURL)) {
		makeErrorResponse(404);
		return;
	}
	if (std::remove(configURL.c_str()) != 0) {
		makeErrorResponse(500);
		return;
	}
	response.makeStatusLine("HTTP/1.1", "200", "OK");
}