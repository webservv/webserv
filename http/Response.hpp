#ifndef RESPONSE_HPP
#define RESPONSE_HPP
#include <string>

class Response {
private:
	std::string	responseStr;
private:
	Response(const Response& copy);
	Response&	operator=(const Response& copy);
public:
	Response();
	~Response();
public:
	void				makeStatusLine(const std::string& version, const std::string& statusCode, const std::string& statusMessage);
	void				makeHeader(const std::string& key, const std::string& value);
	void				makeBody(const std::string& data, const size_t len, const std::string& type);
	const std::string&	getResponseStr(void) const;
};

#endif
