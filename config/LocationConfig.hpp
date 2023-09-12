#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <queue>
#include <vector>
#include <string>

class ServerConfig;

class LocationConfig {
private:
    std::string             	URL;
    std::string             	alias;
    std::vector<std::string>	index;
	std::vector<std::string>	limitExcept;
    int                     	returnCode;
    std::string             	returnURL;
    std::string             	CgiPath;
    std::vector<std::string>	CgiLimit;
    bool                    	autoIndex;
    size_t                     	clientMaxBodySize;
//LocationConfig.cpp
public:
	LocationConfig();
	LocationConfig(const ServerConfig& server);
	LocationConfig(const LocationConfig& src);
	~LocationConfig();
	LocationConfig& operator=(const LocationConfig& src);
private:
	bool	isValidMethod(const std::string& method) const;
	bool	isNumber(const std::string& str) const;
public:
	void	parseURL(std::queue<std::string>& tokens);
	void	parseAlias(std::queue<std::string>& tokens);
	void	parseIndex(std::queue<std::string>& tokens);
	void	parseLimitExcept(std::queue<std::string>& tokens);
	void	parseAutoIndex(std::queue<std::string>& tokens);
	void	parseReturn(std::queue<std::string>& tokens);
	void	parseCgiPath(std::queue<std::string>& tokens);
	void	parseCgiLimit(std::queue<std::string>& tokens);
	void	parseClientMaxBodySize(std::queue<std::string>& tokens);
public:
	const std::string&				getURL(void) const;
	const std::string&				getAlias(void) const;
	const std::vector<std::string>&	getIndex(void) const;
	const std::vector<std::string>&	getLimitExcept(void) const;
	int								getReturnCode(void) const;
	const std::string&				getReturnURL(void) const;
	const std::string&				getCgiPath(void) const;
	const std::vector<std::string>&	getCgiLimit(void) const;
	bool							getAutoIndex(void) const;
	size_t							getClientMaxBodySize(void) const;

};


#endif