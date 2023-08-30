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
		OTHER
	};
private:
	std::string							requestStr;
	std::queue<std::string>				requestLines;
	METHOD								method;
	std::map<std::string, std::string>	values;
	bool								haveHeader;
    int									error;
	size_t								bodyPos;
//Reqeust_parse.cpp
private:
    void	parseMethod(std::string& line);
    void	parseURL(const std::string& line);
    void	parseVersion(const std::string& line, const size_t space);
	void	parseBody(void);
    void	addRequestLines(void);
    void	parseChunkedBody(void);
	void	parseRequestLine(void);
	void	parseKeyValues(void);
	void	parseHeader(void);
public:
    void	parse(void);
//Request.cpp
public:
	Request();
	~Request();
	Request(const Request& copy);
	Request&	operator=(const Request& copy);
public:
	Request::METHOD		getMethod(void) const;
	const std::string&	getRequestStr(void) const;
	const std::string&	getStrMethod(void) const;
	const std::string&	getURL(void) const;
	const std::string&	getBody(void) const;
	const std::string&	getVersion(void) const;
    int					getError(void) const;
	const std::string&	findValue(const std::string& headerName) const;
	void				addRequest(const std::string& request);
    bool				isHeaderEnd(void);
	bool				isRequestEnd(void) const;
};

#endif
