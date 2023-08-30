#include "Router.hpp"
#include "Server.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

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

bool Router::resourceExists(const std::string& filePath) const {
    return !access(filePath.c_str(), F_OK);
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

void Router::setParsedURL() {
    const std::string& URLFromRequest = request.getURL();
    std::string bestMatchURL;
    std::string bestMatchRoot;

    for (iter it = config->locations.begin(); it != config->locations.end(); ++it) {
        const std::string& url = it->url;
        if (URLFromRequest.find(url) == 0) {
            if (url.length() > bestMatchURL.length()) {
                bestMatchURL = url;
                bestMatchRoot = it->root;
            }
        }
    }
    if (!bestMatchURL.empty()) {
        configURL = bestMatchRoot + URLFromRequest.substr(bestMatchURL.length());
    } else {
        if (URLFromRequest == "/") {
            for (size_t i = 0; i < config->index.size(); ++i) {
                std::string potentialIndexPath = config->root_path + config->index[i];
                if (access(potentialIndexPath.c_str(), R_OK)) {
                    configURL = potentialIndexPath;
                    break;
                }
            }
        }  else {
            configURL = config->root_path + URLFromRequest;
        }
    }
}

void Router::parseURL(void) {
    const std::string&  url = configURL;
    std::cout << "url: " << url << std::endl;
    std::cout << "root_path: " << config->root_path << std::endl;
    const size_t        pathIndex = url.find('/', configURL.length());
    const size_t        queryIndex = url.find('?');

    if (pathIndex != std::string::npos)
        CgiVariables["SCRIPT_NAME"] = url.substr(0, pathIndex);
    else
        CgiVariables["SCRIPT_NAME"] = url.substr(0, queryIndex);
    if (pathIndex != std::string::npos)
        CgiVariables["PATH_INFO"] = url.substr(pathIndex, queryIndex - pathIndex);
    if (queryIndex != std::string::npos)
        CgiVariables["QUERY_STRING"] = url.substr(queryIndex + 1, -1);

    std::cout << "SCRIPT_NAME: " << CgiVariables["SCRIPT_NAME"] << std::endl;
    std::cout << "PATH_INFO: " << CgiVariables["PATH_INFO"] << std::endl;
    std::cout << "QUERY_STRING: " << CgiVariables["QUERY_STRING"] << std::endl;

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
    
    if (it->second == "/cgi/index.py") //hard coding until config file.
        return true;
    return false;
}
