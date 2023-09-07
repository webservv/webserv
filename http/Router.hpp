#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"

#include <exception>
#include <netinet/in.h>
#include <string>
#include <map>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

static const std::string    CGI_PATH = "./document/cgi";

class Server;

class Router {
public:
    enum    eFileType {
        dir,
        file,
        other
    };
public:
    class ErrorException: public std::exception {
    private:
        int         errorCode;
        std::string message;
    public:
        ErrorException(const int errorCode, const std::string& message);
        virtual ~ErrorException() throw();
    public:
        virtual const char* what(void) const throw();
        int                 getErrorCode(void) const;
    };
private:
    static std::map<std::string, std::string>   mimeMap;
private:
	Request		                        request;
	Response	                        response;
    bool                                haveResponse;
    Server*                             server;
    sockaddr_in                         clientAddr;
    const ServerConfig*                 config;
    std::map<std::string, std::string>  CgiVariables;
    std::string                         configURL;
    const LocationConfig*               matchLocation;
// Router.autoindex.cpp
private:
    Router::eFileType   getFileType(const char* path) const;
    const std::string   makeSpacedStr(const int desiredSpaces, std::string target) const;
    
public:
    std::string generateDirectoryListing(const std::string& directoryPath);               
//Router_error.cpp
private:
    std::pair<std::string, std::string> \
            defaultErrorPage(int statusCode);
    void    setCustomErrorPage(const std::string& customPath);
    void    makeDefaultErrorResponse(int statusCode);
public:
    void    makeErrorResponse(int statusCode);
//Router_utils.cpp
private:
    static void         initializeMimeMap();
    const std::string&  findMimeType(const std::string& extension) const;
	const std::string&  getMIME(const std::string& url) const;
    void                readFile(const std::string& filePath, std::vector<char>& outContent) const;
    void                makeCgiVariables(void);
    void                validateHeaderLength(void);
    void                validateContentType(void);
    void                handleDirectory(std::string& UrlFromRequest);
    void                replaceURL(const std::string& UrlFromRequest);
    void                setConfigURL(void);
    void                parseURL(void);
    std::string         intToIP(in_addr_t ip) const;
    bool                needCookie(void) const;
    void                getBestMatchURL(const std::vector<LocationConfig>& locations, const std::string& UrlFromRequest);
    bool                isDirectory(const std::string& path) const;
    bool                isRegularFile(const std::string& path) const;
    bool                isInvalidBodySize(void) const;
//Router_static.cpp
private:
    void    processStaticGet(void);
    void    processStaticPost(void); //WIP
    void    processStaticPut(void); //WIP
    void    processStaticDelete(void); //WIP
//Router.cpp
public:
	Router();
    Router(Server* const server, const sockaddr_in& clientAddr, const ServerConfig* config);
	Router(const Router& src);
	Router&	operator=(const Router& src);
	~Router();
private:
	void				handleGet(void);
    void				handlePost(void);
    void				handleDelete(void);
    void                handlePut(void);
    void                connectCGI(void);
    bool                isBodyRequired(void) const;
    const std::string&  getParsedURL(void) const;
    void                handleMethod(Request::METHOD method);
public:
    void                        handleRedirect(const std::string& url);
	void				        handleRequest(void);
    const ServerConfig*         getConfig(void) const;
    const sockaddr_in&          getClientAddr(void) const;
    bool                        isHeaderEnd(void);
    bool                        isRequestEnd(void) const;
    void                        parseRequest(void);
    bool                        getHaveResponse(void) const;
    const std::vector<char>&    getRequest(void) const;
    const std::vector<char>&    getResponse(void) const;
    size_t                      getSentLength(void) const;
    void                        setSentLength(const size_t size);
    void                        addRequest(const std::vector<char>& input);
    void                        setResponse(const std::vector<char>& src);
    void                        readFromCGI(void);
    void                        writeToCGI(const intptr_t fdBufferSize);
    void                        disconnectCGI(void);
    int                         getWriteFD(void) const;
    int                         getReadFD(void) const;
};

#endif
