#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <unordered_map>
#include <sstream>
#include <queue>

class Request {
public:
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
private:
	Request(const Request& copy);
	Request&	operator=(const Request& copy);
public:
	Request (const std::string& request);
	Request ();
private:
    void parseMethod(std::string& line);
    void parseURL(std::string& line);
    void parseVersion(std::string& line, size_t space);
	void parseRequestLine();
	void parseHeaders();
	void parseBody();
public:
	Request::METHOD getMethod() ;
	const std::string& getBody() const;
	const std::string& getUrl() ;
	const std::string& getHeaderValue(const std::string& headerName) const;
// canonical-form ? 

};

#endif
