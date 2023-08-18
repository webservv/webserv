#include "Response.hpp"
#include <sstream>

Response::Response(): responseStr("") {}

Response::Response(const Response& copy): responseStr(copy.responseStr) {}

Response& Response::operator=(const Response& copy) {
	responseStr = copy.responseStr;
	return *this;
}

Response::~Response() {}

void Response::makeStatusLine(const std::string& version, const std::string& statusCode, const std::string& statusMessage) {
	responseStr += version + " " + statusCode + " " + statusMessage + "\r\n";
}

void Response::makeHeader(const std::string& key, const std::string& value) {
	responseStr += key + ": " + value + "\r\n";
}

void Response::makeBody(const std::string& data, const size_t len, const std::string& type) {
	std::stringstream	ss;
	
	ss << len;
	responseStr += "Content-Length: " + ss.str() + "\r\n";
	responseStr += "Content-Type: " + type + "\r\n";
	responseStr += "\r\n";
	responseStr += data;
}

const std::string& Response::getResponseStr(void) const {
	return responseStr;
}

void Response::setResponse(const std::string &src) {
	responseStr.assign(src);
}