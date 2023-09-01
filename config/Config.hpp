#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <queue>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <map>

class Config {
public:
    struct location {
    std::vector<std::string>            allowedMethod;
    std::string                         url;
    std::string                         root;
    std::vector<std::string>            index;
    std::string                         return_url;
    bool                                autoindex;
    int                                 return_code;
    };
struct server{
	std::vector<location>           locations;
	std::map<int, std::string>      errorPages;
    std::vector<std::string>        index;
	std::string                     server_name;
    std::string                     root;
	int                             listen_port;
    };
private:
	std::queue<std::string>		tokens;
	std::vector<Config::server> servers;
    int                         clientMaxBodySize;
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
    void    parseListen(server& server);
    void    parseServerName(server& server);
    void    parseRoot(server& server);
    void    parseIndex(server& server);
	void    parseErrorPage(server& server);
    void    parseClientMaxBodySize();

	void    parseLocation(server& server);
    void    parseLimitExcept(location& location);
    void    parseRoot(location& location);
    void    parseIndex(location& location);
    void    parseAutoIndex(location& location);
    void    parseReturn(location& location);
    void    trim(std::string &str) const;
	void    tokenization(const std::string& line);
public:
    const std::vector<Config::server>&  getServers() const;
    int                                 getClientMaxBodySize() const;

};

#endif
