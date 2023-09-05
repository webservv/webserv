#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <queue>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <map>

#include "LocationConfig.hpp"
#include "ServerConfig.hpp"

class Config {
private:
	std::queue<std::string>		tokens;
	std::vector<ServerConfig>   servers;
    bool                        hasHTTP;
//Config.cpp
private:
    Config();
	Config(const Config& copy);
	Config& operator=(const Config& copy);
public:
	Config(const std::string& config_file);
	~Config();
private:
    void    parseLine(std::fstream& configParser);
	void    parseHTTP();
	void    parseServer();
    void    trim(std::string &str) const;
	void    tokenization(const std::string& line);
public:
    const std::vector<ServerConfig>&    getServers() const;
    int                                 getClientMaxBodySize() const;

};

#endif
