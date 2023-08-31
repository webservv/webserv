#include "Router.hpp"
#include <fstream>
#include <stdexcept>
#include <vector>

void Router::processStaticGet(const std::string& scriptName) {
	if (!resourceExists(scriptName)) {
		makeErrorResponse(404);
		return;
	}
	std::string content;
    readFile(configURL, content);
    std::string mimeType = getMIME(scriptName);
    response.makeStatusLine("HTTP/1.1", "200", "OK");
    response.makeBody(content, content.size(), mimeType);
	haveResponse = true;
}

void Router::processStaticPost(const std::string& scriptNmae) {
	std::string					path;
	const std::vector<char>&	body = request.getBody();
	std::ofstream				os;

	if (scriptNmae == "/")
		path = "./document/test.txt";
	else
		path = '.' + scriptNmae;
	os.open(path.c_str());
	if (os.fail())
		throw std::runtime_error("processStaticPost: open failure");
	os.write(body.data(), body.size());
	if (os.fail())
		throw std::runtime_error("processStaticPost: write failure");
	response.makeStatusLine("HTTP/1.1", "200", "OK");
	haveResponse = true;
}