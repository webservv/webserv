#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "Request.hpp"
#include "Response.hpp"
#include <string>
#include <map>

class Router {
private:
	Request		request;
	Response	response;
    static std::map<std::string, std::string> mimeMap;
    static void initializeMimeMap();

private:
	Router();
	Router(const Router& src);
	Router&	operator=(const Router& src);
    std::string getExtension(const std::string& url);
    std::string findMimeType(const std::string& extension);
public:
	Router(std::string requestStr);
	~Router();
public:
	void				handleRequest(void);
	void				handleGet(void);
	std::string			getMIME(std::string url);
	const std::string&	getResponseStr(void) const;
};

#endif
