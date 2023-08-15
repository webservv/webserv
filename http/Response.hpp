#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <unordered_map>
#include <sstream>
#include <queue>

class Request {

enum METHOD {
	GET,
	POST,
	DELETE,
	OTHER // -> error or maybe we make other methods
};

private:
	std::istringstream requestParser;
	std::queue<std::string> requestLines;

	METHOD method;
	std::string url;
	std::string version;

	std::unordered_map<std::string, std::string> headers;
	std::string body;

public:
	Request ( std::string request );

	void parseRequstLine();
	void parseHeaders();
	void parseBody();


};

#endif
