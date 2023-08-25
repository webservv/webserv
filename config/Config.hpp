#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <queue>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <map>

struct location {
	std::map<std::string, std::string> info;
	std::string url;
	std::vector<std::string> allowedMethod;
	bool autoIndex;
};

struct server{
	int listen_port;
	std::string server_name;
	std::vector<location> locations;
	std::map<int, std::string> errorPages;
	int clinetMaxSize;
};

class Config {

private:
	std::queue<std::string>		tokens;
	std::vector<server>			servers;
private:
	Config(const Config& copy);
	Config&	operator=(const Config& copy);

private:
	void parseHTTP();
	void parseServer();
	void parseLocation(server& server);
	void parseErrorPage(server& server);
	void tokenization(const std::string& line);

public:
	Config (const std::string& config_file);
	~Config ();

};

#endif
