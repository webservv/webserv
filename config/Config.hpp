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
    std::map<std::string, std::string>  fastcgi_params; // find cgi route 
    std::vector<std::string>            allowedMethod;
    std::vector<std::string>            try_files;
    std::string                         url;
    std::string                         root;
    std::vector<std::string>            index;
    std::string                         include_file;
    std::string                         fastcgi_pass;
    std::string                         return_url;
    bool                                is_regex;
    bool                                autoindex;
    int                                 return_code;
    };
struct server{
	std::vector<Config::location>   locations;
	std::map<int, std::string>      errorPages;
    std::vector<std::string>        index;
	std::string                     server_name;
    std::string                     root_path;
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
    void    parseErrorPage();
    void    parseRoot(server& server);
    void    parseIndex(server& server);
    void    parseClientMaxBodySize();
	void    parseLocation(server& server);
	void    parseErrorPage(server& server);
    void    parseLimitExcept(location& location);
    void    parseTryFiles(location& location);
    void    parseRoot(location& location);
    void    parseIndex(location& location);
    void    parseAutoIndex(location& location);
    void    parseInclude(location& location);
    void    parseFastcgiPass(location& location);
    void    parseFastcgiParam(location& location);
    void    parseReturn(location& location);
    void    trim(std::string &str) const;
	void    tokenization(const std::string& line);
public:
    const std::vector<server>&  getServers() const;
    int                         getClientMaxBodySize() const;

};

#endif
