#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "Request.hpp"
#include "Response.hpp"
#include <string>
#include <map>
#include <sys/socket.h>
#include <unistd.h>

class Router {
private:
	Request		request;
	Response	response;
    int         clientSocket;
    static std::map<std::string, std::string> mimeMap;
    static void initializeMimeMap();

private:
	Router();
	Router(const Router& src);
	Router&	operator=(const Router& src);
    std::string getExtension(const std::string& url);
    std::string findMimeType(const std::string& extension);
    bool resourceExists(const std::string& filePath);
    void parseURL(std::string& filePath);
    void readFile(const std::string& filePath, std::string& content);
    void sendResponse(const std::string& responseStr);
	void sendErrorPage(void);
public:
	Router(const std::string& requestStr, const int clientSocket);
	~Router();
public:
	void				handleRequest(void);
	void				handleGet(void);
    void				handlePost(void);
    void				handleDelete(void);
	std::string         getMIME(const std::string& url);
	const std::string&	getResponseStr(void) const;
};

#endif
