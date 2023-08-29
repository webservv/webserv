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
    static void initializeMimeMap();
private:
	Request		                        request;
	Response	                        response;
    bool                                haveResponse;
    Server*                             server;
    sockaddr_in                         clientAddr;
    const Config::server*               config;
    std::map<std::string, std::string>  CgiVariables;
    std::string                         parsedURL;
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
	const std::string&  getMIME(const std::string& url) const;
	const std::string&	getResponseStr(void) const;
    void                connectCGI(void);
    std::string         intToIP(in_addr_t ip) const;
    std::string         getExtension(const std::string& url) const;
    const std::string&  findMimeType(const std::string& extension) const;
    bool                resourceExists(const std::string& filePath) const;
    void                readFile(const std::string& filePath, std::string& content) const;
	void                makeErrorPage(void);
    void                validateHeaderLength(void);
    void                validateContentType(void);
    bool                isBodyRequired(void) const;
    void                makeCgiVariables(void);
    void                setParsedURL(void);
    const std::string&  getParsedURL(void) const;
    void                parseURL(void);
    bool                needCookie(void) const;
public:
	void				    handleRequest(void);
    bool                    isHeaderEnd(void) const;
    bool                    isRequestEnd(void) const;
    bool                    getHaveResponse(void) const;
    const std::string&      getResponse(void) const;
    const Config::server*   getConfig(void) const;
    void                    addRequest(const std::string& request);
    void                    setResponse(const std::string& src);
    void                    makeErrorResponse(int statusCode);
    void                    readCGI(void);
    void                    writeCGI(const intptr_t fdBufferSize);
    void                    disconnectCGI(void);
    int                     getWriteFD(void) const;
    int                     getReadFD(void) const;
    int                     getRequestError(void) const;
    const sockaddr_in&      getClientAddr(void) const;
    void                    parseRequest(void);
};

#endif
