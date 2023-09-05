#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <queue>
#include <vector>
#include <string>

class ServerConfig;

class LocationConfig {
private:
    std::string             	URL;
    std::string             	root;
    std::vector<std::string>	index;
	std::vector<std::string>	limitExcept;
    int                     	returnCode;
    std::string             	returnURL;
    std::string             	CgiPath;
    std::vector<std::string>	CgiLimit;
    bool                    	autoindex;
    int                     	clientMaxBodySize;
//LocationConfig.cpp
public:
	LocationConfig();
	LocationConfig(const ServerConfig& server);
	LocationConfig(const LocationConfig& src);
	~LocationConfig();
	LocationConfig& operator=(const LocationConfig& src);
private:
	bool	isValidMethod(const std::string& method) const;
public:
	void	parseURL(std::queue<std::string>& tokens);
	void	parseRoot(std::queue<std::string>& tokens);
	void	parseIndex(std::queue<std::string>& tokens);
	void	parseLimitExcept(std::queue<std::string>& tokens);
	void	parseAutoIndex(std::queue<std::string>& tokens);
	void	parseReturn(std::queue<std::string>& tokens);
	void	parseCgiPath(std::queue<std::string>& tokens);
	void	parseCgiLimit(std::queue<std::string>& tokens);
};


#endif