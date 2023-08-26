#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <queue>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <map>

struct location {
    std::string url;
    bool is_regex;
    std::vector<std::string> allowedMethod;
    std::vector<std::string> try_files;
    std::string root;
    bool autoindex;
    std::string include_file;
    std::string fastcgi_pass;
    std::map<std::string, std::string> fastcgi_params;
    int return_code;
    std::string return_url;
};


struct server{
	int listen_port;
	std::string server_name;
	std::map<int, std::string> errorPages;
    std::string root_path;
    std::string index_file;
	std::vector<location> locations;
};

class Config {

private:
	std::queue<std::string>		tokens;
	std::vector<server>			servers;
    int                         clientMaxBodySize;
private:
	Config(const Config& copy);
	Config&	operator=(const Config& copy);

private:
    void parseLine(std::fstream& configParser);
	void parseHTTP();
	void parseServer();
    void parseListen(server& server);
    void parseServerName(server& server);
    void parseErrorPage();
    void parseRoot(server& server);
    void parseIndex(server& server);
    void parseClientMaxBodySize();
	void parseLocation(server& server);
	void parseErrorPage(server& server);
	void tokenization(const std::string& line);
    void parseLimitExcept(location& location);
    void parseTryFiles(location& location);
    void parseRoot(location& location);
    void parseAutoIndex(location& location);
    void parseInclude(location& location);
    void parseFastcgiPass(location& location);
    void parseFastcgiParam(location& location);
    void parseReturn(location& location);
    std::string trim(const std::string &str);

public:
	Config(const std::string& config_file);
    const std::vector<server>& getServers() const;
    int getClientMaxBodySize() const;
	~Config();

};

#endif
