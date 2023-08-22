#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <sstream>
#include <queue>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <string>

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
	std::map<std::string, std::string> headers;
	std::vector<std::string> bodyLines;
	std::string body;
	bool haveHeader;
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
    void readHeadersAndInitialRequestLines(std::stringstream& parser);
    void handleChunkedTransferEncoding(std::stringstream& parser);
    void handleNonChunkedTransferEncoding(std::stringstream& parser);
    void handleFirstLineOfBody(std::stringstream& parser, std::string& line);
public:
	Request::METHOD getMethod() ;
	const std::vector<std::string>& getBodyLines(void) const;
	const std::string& getBody(void) const;
	const std::string& getUrl() ;
	const std::string& getHeaderValue(const std::string& headerName) const;

	void addRequest(const std::string& request);
	void parseHeader(void);
	void parseBody(void);
    bool isHeaderEnd(void) const;
	bool isRequestEnd(void);
};

#endif
