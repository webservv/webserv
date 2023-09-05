#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <queue>
#include <vector>
#include <map>
#include "LocationConfig.hpp"

class ServerConfig {
private:
	std::string                	serverName;
	int                        	listenPort;
    std::string                	root;
	std::vector<LocationConfig>	locations;
	std::map<int, std::string> 	errorPages;
    std::vector<std::string>   	index;
    size_t                      clientMaxBodySize;
	int							returnCode;
	std::string					returnURL;
//ServerConfig.cpp
public:
	ServerConfig();
	ServerConfig(const ServerConfig& src);
	~ServerConfig();
	ServerConfig&	operator=(const ServerConfig& src);
private:
	bool	isNumber(const std::string& str) const;
public:
	void	parseServerName(std::queue<std::string>& tokens);
	void	parseListenPort(std::queue<std::string>& tokens);
	void	parseRoot(std::queue<std::string>& tokens);
	void	parseIndex(std::queue<std::string>& tokens);
	void	parseLocation(std::queue<std::string>& tokens);
	void	parseErrorPages(std::queue<std::string>& tokens);
	void	parseClientMaxBodySize(std::queue<std::string>& tokens);
public:
	const std::string&					getServerName(void) const;
	int									getListenPort(void) const;
	const std::string&					getRoot(void) const;
	const std::vector<LocationConfig>&	getLocations(void) const;
	const std::map<int, std::string>&	getErrorPages(void) const;
	const std::vector<std::string>&		getIndex(void) const;
	size_t								getClientMaxBodySize(void) const;
	int									getReturnCode(void) const;
	const std::string&					getReturnURL(void) const;
};

#endif