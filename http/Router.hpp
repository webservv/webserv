#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "Request.hpp"
#include "Response.hpp"

#include <string>
#include <map>
#include <sys/socket.h>
#include <unistd.h>

class Server;

class Router {
private:
	Request		request;
	Response	response;
    bool        haveResponse;
    Server*     server;
private:
    static std::map<std::string, std::string> mimeMap;
    static void initializeMimeMap();
private:
    std::string getExtension(const std::string& url);
    std::string findMimeType(const std::string& extension);
    bool resourceExists(const std::string& filePath);
    void parseURL(std::string& filePath);
    void readFile(const std::string& filePath, std::string& content);
	void makeErrorPage(void);
    std::string readPosts(void);
    std::string URLDecode(const std::string& str);
    void validateHeaderLength(void);
    void validateContentType(void);
    bool isBodyRequired(void);
    void parsePostData(std::string& title, std::string& postContent);
    void appendPostToFile(const std::string& title, const std::string& postContent);
    void readAndModifyHTML(std::string& htmlResponse);
    void makeHTMLResponse(const std::string& htmlResponse);
public:
	Router();
    Router(Server* const server);
	Router(const Router& src);
	Router&	operator=(const Router& src);
	~Router();
private:
	void				handleGet(void);
    void				handlePost(void);
    void				handleDelete(void);
	std::string         getMIME(const std::string& url);
	const std::string&	getResponseStr(void) const;
public:
	void				handleRequest(void);
    bool                isHeaderEnd(void);
    bool                isRequestEnd(void);
    bool                getHaveResponse(void) const;
    const std::string&  getResponse(void) const;
    void                addRequest(const std::string& request);
    void                parseHeader(void);
    void                parseBody(void);
    void                setResponse(const std::string& src);
    void                makeErrorResponse(int statusCode);
    void                readCGI(void);
    void                writeCGI(const intptr_t fdBufferSize);
    void                disconnectCGI(void);
    int                 getWriteFd(void) const;
    int                 getReadFd(void) const;
};

#endif
