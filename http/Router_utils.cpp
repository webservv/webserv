#include "Router.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

#define MAX_POST_SIZE 500 * 1024 * 1024

static std::string findPotentialIndexPath(const std::string& rootPath, \
    const std::vector<std::string>& indexFiles, const std::string& url);
static void GetBestMatchURL(
    const std::vector<Config::location>& locations,
    const std::string& URLFromRequest,
    std::string& bestMatchURL,
    std::string& bestMatchRoot,
    Config::location& bestLocation
);
static std::string findPath(const std::string& rootPath, const std::string& url);

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

void Router::readFile(const std::string& filePath, std::string& content) const {
    std::ifstream ifs(filePath.c_str());
    if (!ifs.is_open()) {
        throw std::ios_base::failure("File open error.");
    }

    std::string line;
    content.clear();
    while (std::getline(ifs, line)) {
        content += line;
        content += "\n";
    }
    ifs.close();
}

void Router::makeCgiVariables(void) {
    std::stringstream   ss;
    const std::string&  host = request.findValue("host");
    const size_t        portIndex = host.find(':');

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
    CgiVariables["SERVER_HOST"] = host.substr(portIndex + 1, -1);
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
            makeErrorResponse(411);
            throw std::runtime_error("Content-Length header is missing");
        }
        return;
    }
    try {
        int contentLength = std::stoi(contentLengthHeader);
        if (contentLength < 0) {
            makeErrorResponse(400);
            throw std::runtime_error("Content-Length is not a valid integer");
        }
        else if (contentLength > MAX_POST_SIZE) {
            makeErrorResponse(413);
            throw std::runtime_error("Content-Length is too large");
        }
    } catch (const std::invalid_argument& e) {
        makeErrorResponse(400);
        throw std::runtime_error("Content-Length is not a valid integer");
    }
}

void Router::validateContentType() {
    const std::string& contentType = request.findValue("Content-Type");
    const std::string& transferEncoding = request.findValue("Transfer-Encoding");

    if (!transferEncoding.empty()) {
        if (transferEncoding != "chunked") {
            makeErrorResponse(501);
            throw std::runtime_error("Invalid Transfer-Encoding value");
        }
        return;
    }
    if (!contentType.empty() && contentType != "application/x-www-form-urlencoded") {
        makeErrorResponse(415);
        throw std::runtime_error("Unsupported Media Type");
    }
}

void Router::parseDirectory(std::string& URLFromRequest, const std::string& bestMatchRoot, const Config::location& bestLocation, std::string& configURL, std::string& configRoot) {
    URLFromRequest.erase(URLFromRequest.end() - 1);
    
    throw std::runtime_error("for test");
    // directory index를 찾는 로직인데 나중에 테스트 하면서 고쳐야 됩니다. 
    // directory가 들어왔을 때 bestMatchRoot로 돌리는게 맞는가..? 의문
    if (bestMatchRoot.empty()) {
        configURL = findPotentialIndexPath(config->root, config->index, URLFromRequest);
        configRoot = config->root;
    } else {
        configURL = findPotentialIndexPath(bestMatchRoot, bestLocation.index, URLFromRequest);
        configRoot = bestMatchRoot;
    }
}


void Router::setConfigURL() {
    std::string&        URLFromRequest = const_cast<std::string&>(request.getURL());
    std::string         bestMatchURL;
    std::string         bestMatchRoot;
    Config::location    bestLocation;
    
    GetBestMatchURL(config->locations, URLFromRequest, bestMatchURL, bestMatchRoot, bestLocation);
    if (URLFromRequest.back() == '/') {
        try {
            parseDirectory(URLFromRequest, bestMatchRoot, bestLocation, configURL, configRoot);
        } catch (const std::exception& e) {
            // /happy 
            // root /document 
            // -> /document/happy 
            std::cout << "cath!!" << std::endl;
            std::string directoryPath;
            if (bestMatchRoot.empty())
                directoryPath = config->root + URLFromRequest;
            else
                directoryPath = bestMatchRoot + URLFromRequest;
            configURL = generateDirectoryListing(directoryPath);
            configRoot = directoryPath;
            // 디렉토리에서 index 접근을 확인하는 과정에서 실패하면 여기로 들어옴
        }
        return ;
    }
    if (bestMatchRoot.empty()) {
        configURL = findPath(config->root, URLFromRequest);
        configRoot = config->root;
    } else {
        configURL = findPath(bestMatchRoot, URLFromRequest);
        configRoot = bestMatchRoot + bestMatchURL;
    }
}

void Router::parseURL() {
    const std::string& url = configURL;
    std::string path_info, query_string;

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

    CgiVariables["SCRIPT_NAME"] = configURL.substr(1, queryIndex - 1);
    CgiVariables["PATH_INFO"] = path_info;
    CgiVariables["QUERY_STRING"] = query_string;

    std::cout << "configURL: " << configURL << std::endl;
    std::cout << "configRoot: " << configRoot << std::endl;
    std::cout << "SCRIPT_NAME: " << CgiVariables["SCRIPT_NAME"] << std::endl;
    std::cout << "path_info: " << path_info << std::endl;
    std::cout << "query_string: " << query_string << std::endl;
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

static std::string findPotentialIndexPath(const std::string& rootPath, \
    const std::vector<std::string>& indexFiles, const std::string& directory) {
    std::string potentialIndexPath = "." + rootPath + directory;

    potentialIndexPath = potentialIndexPath + "/";
    for (size_t i = 0; i < indexFiles.size(); ++i) {
        potentialIndexPath += indexFiles[i];
        if (access(potentialIndexPath.c_str(), R_OK) == 0) {
            return potentialIndexPath;
        }
    }
    return "." + rootPath + directory;
}

static std::string findPath(const std::string& rootPath, const std::string& url) {
    std::string potentialIndexPath = "." + rootPath + url;
    return potentialIndexPath;
}

static void GetBestMatchURL(
    const std::vector<Config::location>& locations,
    const std::string& URLFromRequest,
    std::string& bestMatchURL,
    std::string& bestMatchRoot,
    Config::location& bestLocation
) {
    for (std::vector<Config::location>::const_iterator it = locations.begin(); 
         it != locations.end(); ++it) {
        const std::string& url = it->url;
        if (URLFromRequest.find(url) == 0) {
            if (url.length() > bestMatchURL.length()) {
                bestMatchURL = url;
                bestMatchRoot = it->root;
                bestLocation = *it;
            }
        }
    }
}
