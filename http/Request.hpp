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
private:
	std::vector<char>					requestStr;
	std::map<std::string, std::string>	values;
	std::vector<char>					body;
	std::string							valueBuffer;
	size_t								readPos;
	size_t								chunkSize;
	size_t								valueStart;
	bool								haveHeader;
	bool								haveBody;
//Reqeust_parse.cpp
private:
	bool	getValueSize(const std::string& spacer);
    bool	splitHeader(const std::string& spacer);
	bool	parseRequestLine(void);
	void	toLower(const std::string& src, std::string& out) const;
	bool	parseKeyValues(void);
	void	parseHeader(void);
	void	parseBody(void);
	void	skipCRLF(void);
	size_t	hexToDecimal(char digit) const;
	bool	parseChunkSize(void);
    void	parseChunkedBody(void);
public:
    void	parse(void);
//Request.cpp
public:
	Request();
	~Request();
	Request(const Request& copy);
	Request&	operator=(const Request& copy);
private:
	size_t	findHeaderEnd(void) const;
public:
	const std::vector<char>&	getRequestStr(void) const;
	const std::string&			getMethod(void) const;
	const std::string&			getURL(void) const;
	const std::vector<char>&	getBody(void) const;
	const std::string&			getVersion(void) const;
	const std::string&			findValue(const std::string& headerName) const;
	void						addRequest(const std::vector<char>& input);
	bool						isRequestEnd(void) const;
};

#endif
