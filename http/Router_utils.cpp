#include "Router.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

static const std::string	g_dir = "./document";
static const std::string    g_error_dir = g_dir + "/error.html";
static const std::string    post_txt = g_dir + "/posts.txt";
static const std::string    index_html = g_dir + "/index.html";

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

std::string Router::getExtension(const std::string& url) {
    size_t extensionStart = url.find_last_of('.');
    if (extensionStart == std::string::npos) {
        return "";
    }
    return url.substr(extensionStart + 1);
}

std::string Router::findMimeType(const std::string& extension) {
    std::map<std::string, std::string>::const_iterator it = mimeMap.find(extension);
    if (it != mimeMap.end()) {
        return it->second;
    } else {
        return "application/octet-stream";
    }
}

std::string Router::getMIME(const std::string& url) {
    const std::string& extension = getExtension(url);
    return findMimeType(extension);
}

void Router::parseURL(std::string& filePath) {
    std::string urlPath = request.getUrl();

    if (urlPath == "/")
        filePath = g_dir + "/index.html";
    else if (!urlPath.compare(0, 4, "/cgi"))
        filePath = urlPath;
    else
        filePath = g_dir + urlPath;
}

bool Router::resourceExists(const std::string& filePath) {
    return !access(filePath.c_str(), F_OK);
}

void Router::readFile(const std::string& filePath, std::string& content) {
    std::ifstream ifs(filePath.c_str());
    if (!ifs.is_open()) {
        throw std::ios_base::failure("File open error.");
    }
    content.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
}

const std::string& Router::getResponseStr(void) const {
	return response.getResponseStr();
}

void Router::makeErrorPage(void) {
    makeErrorResponse(404);
    if (resourceExists(g_error_dir)) {
        std::string data;
        readFile(g_error_dir, data);
        response.makeBody(data, data.length(), getMIME(g_error_dir));
    }
}

std::string Router::URLDecode(const std::string &input) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '%') {
            int value;
            std::istringstream is(input.substr(i + 1, 2));
            is >> std::hex >> value;
            oss << static_cast<char>(value);
            i += 2;
        } else if (input[i] == '+') {
            oss << ' ';
        } else {
            oss << input[i];
        }
    }
    return oss.str();
}

bool Router::isBodyRequired() {
    Request::METHOD method = request.getMethod();
    switch (method) {
        case Request::POST:
            return true;
        default:
            return false;
    }
}

void Router::validateHeaderLength() {
    const std::string& contentLengthHeader = request.getHeaderValue("Content-Length");
    const std::string& transferEncodingHeader = request.getHeaderValue("Transfer-Encoding");

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
    } catch (const std::invalid_argument& e) {
        makeErrorResponse(400);
        throw std::runtime_error("Content-Length is not a valid integer");
    }
}

void Router::validateContentType() {
    const std::string& contentType = request.getHeaderValue("Content-Type");
    const std::string& transferEncoding = request.getHeaderValue("Transfer-Encoding");

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

void Router::parsePostData(std::string& title, std::string& postContent) {
	const std::string& content = request.getBody();
	std::stringstream ss(content);
	std::string key;
	std::string value;

    if (content == "0" || content.size() == 0) {
        makeErrorResponse(405);
        throw std::runtime_error("Post data cannot be empty");
    }
	while (std::getline(ss, key, '=')) {
		std::getline(ss, value, '&');
		if (key == "title") title = URLDecode(value);
		else if (key == "content") postContent = URLDecode(value);
        else {
            makeErrorResponse(400);
            throw std::runtime_error("Invalid post data");
        }
	}

    if (postContent.size() == 0) {
        makeErrorResponse(400);
        throw std::runtime_error("Post content cannot be empty");
    }
}

void Router::appendPostToFile(const std::string& title, const std::string& postContent) {
    std::ofstream outFile(post_txt.c_str(), std::ios::app);
    if (!outFile) {
        std::cerr << "Could not open or create posts.txt" << std::endl;
        makeErrorResponse(500);
        throw std::runtime_error("File error");
    }
    outFile << "Title: " << title << "\nContent: " << postContent << "\n\n";
    outFile.close();
}

void Router::readAndModifyHTML(std::string& htmlResponse) {
    std::ifstream inFile(index_html.c_str());
    if (!inFile.is_open()) {
        std::cerr << "Error opening " << index_html << std::endl;
        makeErrorResponse(500);
        throw std::runtime_error("File open error");
    }

    std::ostringstream oss;
    char c;
    while (inFile.get(c)) {
        oss.put(c);
    }
    htmlResponse = oss.str();
    inFile.close();

    std::string commentStr = "<!-- You can add forums, threads, posts, etc. here -->";
    std::size_t commentPos = htmlResponse.find(commentStr);
    if (commentPos != std::string::npos) {
        std::string postsHtml = readPosts();
        htmlResponse.replace(commentPos, commentStr.length(), postsHtml);
    } else {
        std::cerr << "Placeholder comment not found in HTML file." << std::endl;
    }
}


void Router::makeHTMLResponse(const std::string& htmlResponse) {
	response.makeStatusLine("HTTP/1.1", "200", "OK");
	response.makeBody(htmlResponse, htmlResponse.size(), "text/html");
}

std::string Router::readPosts() {
	std::ifstream file(post_txt);
	std::string postHtml;
	std::string line;

	while (std::getline(file, line)) {
		postHtml += "<p>" + line + "</p>";
	}

	file.close();
	return postHtml;
}

void Router::makeErrorResponse(int statusCode) {
    std::string reasonPhrase;
    std::string body;

    switch (statusCode) {
        case 400:
            reasonPhrase = "Bad Request";
            body = "The request could not be understood by the server due to malformed syntax.";
            break;
        case 401:
            reasonPhrase = "Unauthorized";
            body = "The request requires user authentication.";
            break;
        case 403:
            reasonPhrase = "Forbidden";
            body = "The server understood the request, but is refusing to fulfill it.";
            break;
        case 404:
            reasonPhrase = "Not Found";
            body = "The server has not found anything matching the Request-URI.";
            break;
        case 405:
            reasonPhrase = "Method Not Allowed";
            body = "The method specified in the Request-Line is not allowed for the resource identified by the Request-URI.";
            break;
        case 411:
            reasonPhrase = "Length Required";
            body = "The request did not specify the length of its content, which is required by the requested resource.";
            break;
        case 413:
            reasonPhrase = "Request Entity Too Large";
            body = "The server is refusing to process a request because the request entity is larger than the server is willing or able to process.";
            break;
        case 415:
            reasonPhrase = "Unsupported Media Type";
            body = "The server is refusing to service the request because the entity of the request is in a format not supported by the requested resource for the requested method.";
            break;
        case 500:
            reasonPhrase = "Internal Server Error";
            body = "The server encountered an unexpected condition which prevented it from fulfilling the request.";
            break;
        case 501:
            reasonPhrase = "Not Implemented";
            body = "The server does not support the functionality required to fulfill the request.";
            break;
        case 505:
            reasonPhrase = "HTTP Version Not Supported";
            body = "The server does not support, or refuses to support, the HTTP protocol version that was used in the request message.";
            break;
        default:
            statusCode = 500;
            reasonPhrase = "Internal Server Error";
            body = "An unexpected error occurred.";
            break;
    }

    response.makeStatusLine("HTTP/1.1", std::to_string(statusCode), reasonPhrase);
    response.makeBody(body, body.length(), "text/plain");
    haveResponse = true;
}
