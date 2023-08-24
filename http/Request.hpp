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
	std::map<std::string, std::string> values;
	bool haveHeader;
    int error;
public:
	Request();
	~Request();
	Request(const Request& copy);
	Request&	operator=(const Request& copy);
private:
    void parseMethod(std::string& line);
    void parseURL(const std::string& line);
    void parseVersion(const std::string& line, const size_t space);
	void parseRequestLine();
	void parseKeyValues();
	void addRequestLines(void);
    void readHeadersAndInitialRequestLines(std::stringstream& parser);
    void handleChunkedTransferEncoding(std::stringstream& parser);
    void handleNonChunkedTransferEncoding(std::stringstream& parser);
    void handleFirstLineOfBody(std::stringstream& parser, std::string& line);
public:
	Request::METHOD getMethod(void) const;
	const std::string& getStrMethod(void) const;
	const std::vector<std::string>& getBodyLines(void) const;
	const std::string& findValue(const std::string& headerName) const;
	const std::string& getBody(void) const;
	const std::string& getUrl(void) const;
	const std::string& getPath(void) const;
	const std::string& getQuery(void) const;
	const std::string& getVersion(void) const;

	void addRequest(const std::string& request);
	void parseHeader(void);
	void parseBody(void);
    bool isHeaderEnd(void) const;
	bool isRequestEnd(void);
    int getError(void) const;
};

#endif
