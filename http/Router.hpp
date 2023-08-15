#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "Request.hpp"
#include "Response.hpp"

class Router {
private:
	Request		request;
	Response	response;
private:
	Router();
	Router(const Router& src);
	Router&	operator=(const Router& src);
public:
	Router(std::string requestStr);
	~Router();
public:
	void	handleRequest(void);
	void	handleGet(void);
	std::string getMIME(std::string url);

};

#endif
