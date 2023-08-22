#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <queue>
#include <sstream>
#include <fstream>
#include <algorithm>

struct location {
	std::string url;
	std::string root;
	std::string index;
};

struct server{
	int listen_port;
	std::string server_name;
	std::vector<location> locations;
	std::string error_page; // it can be many and various...
};

class Config {

private:
	std::queue<std::string>		tokens;
	std::vector<server>			servers;
private:
	Config(const Config& copy);
	Config&	operator=(const Config& copy);

private:
	void parseServer();
	void parseLocation(server& server);
	void tokenization(const std::string& line);

public:
	Config (const std::string& config_file);
	~Config ();

};

#endif
