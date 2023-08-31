#include "Router.hpp"
#include <fstream>
#include <stdexcept>
#include <vector>

void Router::processStaticGet(void) {
	if (!resourceExists(configURL)) {
		makeErrorResponse(404);
		return;
	}
	std::string content;
    readFile(configURL, content);
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
	os.open(configURL.c_str());
	if (os.fail())
		throw std::runtime_error("processStaticPost: open failure");
	os.write(body.data(), body.size());
	if (os.fail())
		throw std::runtime_error("processStaticPost: write failure");
	response.makeStatusLine("HTTP/1.1", "200", "OK");
	haveResponse = true;
}