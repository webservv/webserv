#include "Request.hpp"
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
#include <unistd.h>

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

    size_t extensionStart = url.find_last_of('.');
    if (extensionStart == std::string::npos) {
        findMimeType("");
    }
    return findMimeType(url.substr(extensionStart + 1));
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
    CgiVariables["REQUEST_METHOD"] = request.getMethod();
    CgiVariables["SERVER_NAME"] = config->getServerName();
    CgiVariables["SERVER_HOST"] = host.substr(portPos + 1, -1);
    CgiVariables["SERVER_PROTOCOL"] = request.getVersion();
    CgiVariables["SERVER_SOFWARE"] = "webserv/0.42";
    CgiVariables["HTTP_COOKIE"] = request.findValue("cookie");
    CgiVariables["HTTP_USER_AGENT"] = request.findValue("user-agent");
    CgiVariables["HTTP_ACCEPT_LANGUAGE"] = request.findValue("accept-encoding");
    CgiVariables["HTTP_HOST"] = request.findValue("host");
    CgiVariables["HTTP_X_SECRET_HEADER_FOR_TEST"] = request.findValue("x-secret-header-for-test");
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
        && type != "text/plain"
        && type != "image/png") {
        throw Router::ErrorException(415, "Unsupported Media Type");
    }
}

void Router::handleDirectory() {
    const std::vector<std::string>& indexFiles = matchLocation ? matchLocation->getIndex() : config->getIndex();
    const std::string& directoryPath = matchLocation ? matchLocation->getAlias() : config->getAlias();
    std::string testURL;

    if (configURL.back() != '/')
        configURL += "/";

    for (size_t i = 0; i < indexFiles.size(); ++i) {
        testURL = "." + configURL + indexFiles[i];
        if (access(testURL.c_str(), F_OK) == 0) {
            configURL = configURL + indexFiles[i];
            return ;
        }
    }
    
    if (matchLocation && matchLocation->getAutoIndex())
        configURL = generateDirectoryListing(directoryPath);
    else 
        configURL = testURL.erase(0, 1);
}

void Router::replaceURL(const std::string& UrlFromRequest) {
    if (matchLocation) {
        if (matchLocation->getURL().front() == '.') {
            configURL = matchLocation->getCgiPath();
        }
        else {
            configURL = UrlFromRequest;
            if (matchLocation->getURL() == "/")
                configURL = "/" + UrlFromRequest;
            if (matchLocation->getAlias().empty())
                configURL.replace(0, matchLocation->getURL().size(), config->getAlias());
            else 
                configURL.replace(0, matchLocation->getURL().size(), matchLocation->getAlias());
        }
    }
    else {
        configURL = config->getAlias() + UrlFromRequest;
    }
}

void Router::setConfigURL() {
    const std::string& URLFromRequest = request.getURL();
    std::string path;
    
    getBestMatchURL(config->getLocations(), URLFromRequest);
    if (!matchLocation->getReturnURL().empty()) {
        configURL = matchLocation->getReturnURL();
        return ;
    }
    replaceURL(URLFromRequest);
    path = '.' + configURL;
    if (access(path.c_str(), F_OK) || isRegularFile(path))
        ;
    else if (configURL.back() == '/' || isDirectory(path)) {
        handleDirectory();
    }
    else
        throw ErrorException(404, "setConfigURL: not a file or directory");
}

void Router::parseURL() {
    std::string path_info;
    std::string query_string;
    size_t      cgiIndex;
    size_t      pathIndex;
    size_t      queryIndex;

    if (configURL == "/cgi/cgi_tester")  {//tester only
        CgiVariables["PATH_INFO"] = request.getURL();
        CgiVariables["REQUEST_URI"] = request.getURL();
        CgiVariables["SCRIPT_NAME"] = request.getURL();
        return;
    }
    cgiIndex = configURL.find("/cgi/");
    pathIndex = configURL.find("/", cgiIndex + 5);
    queryIndex = configURL.find('?');
    if (pathIndex != std::string::npos) {
        size_t beginIndex = pathIndex;
        size_t endIndex = (queryIndex != std::string::npos) ? queryIndex : configURL.length();
        path_info = configURL.substr(beginIndex, endIndex - beginIndex);
        CgiVariables["SCRIPT_NAME"] = configURL.substr(0, pathIndex);
    }
    if (queryIndex != std::string::npos) {
        query_string = configURL.substr(queryIndex + 1);
        if (CgiVariables.find("SCRIPT_NAME") == CgiVariables.end()) {
            CgiVariables["SCRIPT_NAME"] = configURL.substr(0, queryIndex);
        }
    }
    if (CgiVariables.find("SCRIPT_NAME") == CgiVariables.end()) {
        CgiVariables["SCRIPT_NAME"] = configURL.substr(0, queryIndex - 1);
    }
    CgiVariables["REQEUST_URI"] = request.getURL();
    CgiVariables["PATH_INFO"] = path_info;
    CgiVariables["QUERY_STRING"] = query_string;
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
    const static std::string                            CGI_PATH = "/document/cgi/index.py";
    std::map<std::string, std::string>::const_iterator  it = CgiVariables.find("SCRIPT_NAME");
    if (it->second == CGI_PATH)
        return true;
    return false;
}

void Router::getBestMatchURL(
    const std::vector<LocationConfig>& locations,
    const std::string& UrlFromRequest
) {
    size_t              longestUrlsize = 0;
    std::stringstream   ss;
    size_t              dotPos = UrlFromRequest.rfind('.');
    const std::string   extension = (dotPos != std::string::npos ? UrlFromRequest.substr(dotPos, -1) : "");

    for (std::vector<LocationConfig>::const_iterator it = locations.begin(); 
        it != locations.end(); ++it) {
        const std::string& url = it->getURL();
        if (UrlFromRequest == url) {
            matchLocation = &(*it);
            return;
        }
        else if (!extension.empty() && extension == url) {
            const std::vector<std::string>& methodLimit = it->getCgiLimit();
            if (std::find(methodLimit.begin(), methodLimit.end(), request.getMethod()) == methodLimit.end())
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

bool Router::isInvalidBodySize(void) const {
    size_t  bodySize = request.getBody().size();
    size_t  clientMaxBodySize = matchLocation->getClientMaxBodySize();

    if (clientMaxBodySize == 0)
        return false;
    if (bodySize > clientMaxBodySize)
        return true;
    return false;
}