#include "Router.hpp"
#include "Server.hpp"
#include <cmath>
#include <exception>
#include <string>
#include <sys/_types/_size_t.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>

#define MAX_POST_SIZE 500 * 1024 * 1024

void Router::initializeMimeMap() {
    if (mimeMap.empty()) {
        mimeMap["html"] = "text/html";
        mimeMap["txt"] = "text/plain";
        mimeMap["css"] = "text/css";
        mimeMap["file"] = "text/file";
        mimeMap["js"] = "application/javascript";
        mimeMap["json"] = "application/json";
        mimeMap["xml"] = "application/xml";
        mimeMap["pdf"] = "application/pdf";
        mimeMap["zip"] = "application/zip";
        mimeMap["tar"] = "application/x-tar";
        mimeMap["gif"] = "image/gif";
        mimeMap["png"] = "image/png";
        mimeMap["jpg"] = "image/jpeg";
        mimeMap["ico"] = "image/x-icon";
        mimeMap["py"] = "text/html";
        mimeMap["php"] = "text/html";
        mimeMap["pl"] = "text/html";
    }
}

std::string Router::getExtension(const std::string& url) const {
    size_t extensionStart = url.find_last_of('.');
    if (extensionStart == std::string::npos) {
        return "";
    }
    return url.substr(extensionStart + 1);
}

const std::string& Router::findMimeType(const std::string& extension) const {
    std::map<std::string, std::string>::const_iterator it = mimeMap.find(extension);
    static const std::string    octet_stream = "application/octet-stream";
    if (it != mimeMap.end()) {
        return it->second;
    } else {
        return octet_stream;
    }
}

const std::string& Router::getMIME(const std::string& url) const {
    const std::string& extension = getExtension(url);
    return findMimeType(extension);
}

bool Router::isAccessible(const std::string& filePath) const {
    return !access(filePath.c_str(), F_OK);
}

void Router::readFile(const std::string& filePath, std::vector<char>& outContent) const {
    std::ifstream ifs(filePath.c_str());
    if (!ifs.is_open()) {
        throw Router::ErrorException(500, "File open error.");
    }
    outContent.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
}

void Router::makeCgiVariables(void) {
    std::stringstream   ss;
    const std::string&  host = request.findValue("host");
    const size_t        portPos = host.find(':');

    ss << intToIP(clientAddr.sin_addr.s_addr);
    CgiVariables["AUTH_TYPE"] = request.findValue("AUTH_TYPE");
    CgiVariables["CONTENT_LENGTH"] = request.findValue("content-length");
    CgiVariables["CONTENT_TYPE"] = request.findValue("content-type");
    CgiVariables["GATEWAY_INTERFACE"] = "CGI/1.1";
    CgiVariables["PATH_TRANSLATED"] = request.findValue("PATH_TRANSLATED");
    CgiVariables["REMOTE_ADDR"] = ss.str();
    CgiVariables["REMOTE_HOST"] = request.findValue("REMOTE_HOST");
    CgiVariables["REMOTE_IDENT"] = request.findValue("REMOTE_IDENT");
    CgiVariables["REMOTE_USER"] = request.findValue("REMOTE_USER");
    CgiVariables["REQUEST_METHOD"] = request.getStrMethod();
    CgiVariables["SERVER_NAME"] = config->server_name;
    CgiVariables["SERVER_HOST"] = host.substr(portPos + 1, -1);
    CgiVariables["SERVER_PROTOCOL"] = request.getVersion();
    CgiVariables["SERVER_SOFWARE"] = "webserv/0.42";
    CgiVariables["HTTP_COOKIE"] = request.findValue("cookie");
}

void Router::validateHeaderLength() {
    const std::string& contentLengthHeader = request.findValue("Content-Length");
    const std::string& transferEncodingHeader = request.findValue("Transfer-Encoding");

    if (transferEncodingHeader == "chunked") {
        return;
    }
    if (contentLengthHeader.empty()) {
        if (isBodyRequired()) {
            throw Router::ErrorException(411, "Content-Length header is missing");
        }
        return;
    }
    int contentLength = std::stoi(contentLengthHeader);
    if (contentLength < 0) {
        throw Router::ErrorException(400, "Content-Length is not a valid integer");
    }
    else if (contentLength > MAX_POST_SIZE) {
        throw Router::ErrorException(413, "Content-Length is too large");
    }
}

void Router::validateContentType() {
    const std::string&  transferEncoding = request.findValue("Transfer-Encoding");
    const std::string&  contentType = request.findValue("Content-Type");
    const size_t        secondFieldPos = contentType.find(';');
    const std::string   type = contentType.substr(0, secondFieldPos);

    if (!transferEncoding.empty()) {
        if (transferEncoding != "chunked") {
            throw Router::ErrorException(501, "Invalid Transfer-Encoding value");
        }
        return;
    }
    if (type != "application/x-www-form-urlencoded"
        && type != "multipart/form-data"
        && type != "text/plain") {
        throw Router::ErrorException(415, "Unsupported Media Type");
    }
}

void Router::handleDirectory(std::string& replacedURL) {
    const std::vector<std::string>& indexFiles = matchLocation ? matchLocation->index : config->index;
    const std::string& directoryPath = matchLocation ? matchLocation->root : config->root;
    std::string testURL;

    if (replacedURL.back() != '/')
        replacedURL += "/";

    for (size_t i = 0; i < indexFiles.size(); ++i) {
        testURL = "." + replacedURL + indexFiles[i];
        if (access(testURL.c_str(), F_OK) == 0) {
            configURL = replacedURL + indexFiles[i];
            return ;
        }
    }
    
    if (matchLocation && matchLocation->autoindex)
        configURL = generateDirectoryListing(directoryPath);
    else 
        configURL = testURL.erase(0, 1);
}

void Router::replaceURL(std::string& UrlFromRequest) const {
    if (matchLocation) {
        if (matchLocation->url == "/")
            UrlFromRequest = "/" + UrlFromRequest;
        if (matchLocation->url.front() == '.') {
            UrlFromRequest.assign(matchLocation->CgiPath);
            // configURL = matchLocation->CgiPath;
        }
        else {
            if (matchLocation->root.empty())
                UrlFromRequest.replace(0, matchLocation->url.size(), config->root);
            else 
                UrlFromRequest.replace(0, matchLocation->url.size(), matchLocation->root);
            // configURL = UrlFromRequest;
        }
    }
    else {
        UrlFromRequest = config->root + UrlFromRequest;
    }
}

/*
std::string Router::replaceURL(std::string URLFromRequest) {
    if (matchLocation) {
        if (matchLocation->url == "/")
            URLFromRequest = "/" + URLFromRequest;
        if (matchLocation->root.empty())
            URLFromRequest.replace(0, matchLocation->url.length(), config->root);
        else 
            URLFromRequest.replace(0, matchLocation->url.length(), matchLocation->root);
    }
    else {
        URLFromRequest = config->root + URLFromRequest;
    }
    return URLFromRequest;
}
*/
void Router::setConfigURL() {
    std::string URL = request.getURL();
    std::string path;

    getBestMatchURL(config->locations, URL);
    replaceURL(URL);
    path = '.' + URL;
    if (!isAccessible(path))
        configURL = URL;
    else if (URL.back() == '/' || isDirectory(path)) {
        handleDirectory(URL);
        if (configURL.empty())
            configURL = URL;
    }
    else if (isRegularFile(path)){
        configURL = URL;
    }
    else
        throw ErrorException(404, "setConfigURL: not a file or directory");
}

void Router::parseURL() {
    // have to change a lot here !!! 
    const std::string& url = configURL;
    std::string path_info, query_string;
    std::string configRoot = "trash";

    size_t rootPathIndex = url.find(configRoot);
    size_t queryIndex = url.find('?');

    if (rootPathIndex != std::string::npos) {
        size_t rootPathLength = configRoot.length();
        size_t beginIndex = rootPathIndex + rootPathLength;
        size_t endIndex = (queryIndex != std::string::npos) ? queryIndex : url.length();
        path_info = url.substr(beginIndex, endIndex - beginIndex);
    }

    if (queryIndex != std::string::npos) {
        query_string = url.substr(queryIndex + 1);
    }
    if (url == "/cgi/cgi_tester")  {//tester only
        CgiVariables["PATH_INFO"] = request.getURL();
        CgiVariables["REQUEST_URI"] = request.getURL();
        CgiVariables["SCRIPT_NAME"] = request.getURL();
    }
    else {
        CgiVariables["PATH_INFO"] = "/directory/youpi.bla";
        CgiVariables["REQEUST_URI"] = "/directory/youpi.bla";
        CgiVariables["SCRIPT_NAME"] = configURL.substr(0, queryIndex - 1);
        CgiVariables["PATH_INFO"] = path_info;
        CgiVariables["QUERY_STRING"] = query_string;
    }
}


std::string Router::intToIP(in_addr_t ip) const {
	std::string strIP;
	std::stringstream ss;
	int			tmp = 0;

	for (int i = 0; i < 4; i++) {
		tmp = ip % (1 << 8);
		ip = ip >> 8;
		ss << tmp;
		strIP += ss.str() + '.';
		ss.str("");
	}
	strIP.pop_back();
	return strIP;
}

bool Router::needCookie(void) const {
    std::map<std::string, std::string>::const_iterator it = CgiVariables.find("SCRIPT_NAME");
    if (it->second.substr(0, CGI_PATH.length()) == CGI_PATH)
        return true;
    return false;
}

void Router::getBestMatchURL(
    const std::vector<Config::location>& locations,
    const std::string& UrlFromRequest
) {
    size_t              longestUrlsize = 0;
    std::stringstream   ss;
    const size_t        dotPos = UrlFromRequest.rfind('.');
    const std::string   extension = dotPos != std::string::npos ? UrlFromRequest.substr(dotPos, -1) : "";

    for (std::vector<Config::location>::const_iterator it = locations.begin(); 
        it != locations.end(); ++it) {
        const std::string& url = it->url;
        if (UrlFromRequest == url) {
            matchLocation = &(*it);
            return;
        }
        else if (!extension.empty() && extension == url) {
            const std::vector<std::string>& methodLimit = it->CgiLimit;
            if (std::find(methodLimit.begin(), methodLimit.end(), request.getStrMethod()) == methodLimit.end())
                continue;
            matchLocation = &(*it);
            return;
        }
        else if (UrlFromRequest.find(url) == 0) {
            if (url.size() > longestUrlsize) {
                longestUrlsize = url.size();
                matchLocation = &(*it);
            }
        }
    }
}

bool Router::isDirectory(const std::string& path) const {
    struct stat info;

    if (stat(path.c_str(), &info) != 0) {
        throw Router::ErrorException(500, "isDirectory: " + std::string(strerror(errno)));
    }
    if (S_ISDIR(info.st_mode)) {
        return true;
    }
    return false;
}

bool Router::isRegularFile(const std::string& path) const {
    struct stat info;

    if (stat(path.c_str(), &info) != 0) {
        throw Router::ErrorException(500, "isRegularFile: " + std::string(strerror(errno)));
    }
    if (S_ISREG(info.st_mode)) {
        return true;
    }
    return false;
}
