#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"

#include <netinet/in.h>
#include <string>
#include <map>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

class Server;

class Router {
private:
    static std::map<std::string, std::string>   mimeMap;
private:
	Request		                        request;
	Response	                        response;
    bool                                haveResponse;
    Server*                             server;
    sockaddr_in                         clientAddr;
    const Config::server*               config;
    std::map<std::string, std::string>  CgiVariables;
    std::string                         parsedURL;
//Router_error.cpp
private:
	void    makeErrorPage(void);
public:
    void    makeErrorResponse(int statusCode);
//Router_utils.cpp
private:
    static void         initializeMimeMap();
    std::string         getExtension(const std::string& url) const;
    const std::string&  findMimeType(const std::string& extension) const;
	const std::string&  getMIME(const std::string& url) const;
    bool                resourceExists(const std::string& filePath) const;
    void                readFile(const std::string& filePath, std::string& content) const;
    void                makeCgiVariables(void);
    void                validateHeaderLength(void);
    void                validateContentType(void);
    void                setParsedURL(void);
    void                parseURL(void);
    std::string         intToIP(in_addr_t ip) const;
    bool                needCookie(void) const;
//Router.cpp
public:
	Router();
    Router(Server* const server, const sockaddr_in& clientAddr, const Config::server* config);
	Router(const Router& src);
	Router&	operator=(const Router& src);
	~Router();
private:
	void				handleGet(void);
    void				handlePost(void);
    void				handleDelete(void);
    void                connectCGI(void);
    bool                isBodyRequired(void) const;
	const std::string&	getResponseStr(void) const;
    const std::string&  getParsedURL(void) const;
public:
	void				    handleRequest(void);
    const Config::server*   getConfig(void) const;
    const sockaddr_in&      getClientAddr(void) const;
    bool                    isHeaderEnd(void) const;
    bool                    isRequestEnd(void) const;
    void                    parseRequest(void);
    bool                    getHaveResponse(void) const;
    const std::string&      getResponse(void) const;
    void                    addRequest(const std::string& request);
    void                    setResponse(const std::string& src);
    void                    readFromCGI(void);
    void                    writeToCGI(const intptr_t fdBufferSize);
    void                    disconnectCGI(void);
    int                     getWriteFD(void) const;
    int                     getReadFD(void) const;
    int                     getRequestError(void) const;
};

#endif
