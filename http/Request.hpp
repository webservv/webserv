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
	std::string	requestStr;
	std::queue<std::string> requestLines;

	METHOD method;
	std::string url;
	std::string version;

	std::unordered_map<std::string, std::string> headers;
	std::string body;

public:
	Request();
	~Request();
	Request(const Request& copy);
	Request&	operator=(const Request& copy);
private:
    void parseMethod(std::string& line);
    void parseURL(std::string& line);
    void parseVersion(std::string& line, size_t space);
	void parseRequestLine();
	void parseKeyValues();
	void addRequestLines(void);
public:
	Request::METHOD getMethod() ;
	const std::string& getBody() const;
	const std::string& getUrl() ;
	const std::string& getHeaderValue(const std::string& headerName) const;
	void addRequest(const std::string& request);
	void parseHeader(void);
	void parseBody(void);
    bool isHeaderEnd(void) const;
	bool isRequestEnd(void);
};

#endif
