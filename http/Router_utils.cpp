#include "Router.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

static const std::string	g_dir = "./document";
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

void Router::makeCGIenvs(std::map<std::string, std::string>& envs) const {
    static const std::string requestHeaderList[] = {
        "content-type",
        "content-length",
        "cookie",
        "user-agent",
        ""
    };
    static const std::string envHeaderNames[] = {
        "CONTENT_TYPE",
        "CONTENT_LENGTH",
        "HTTP_COOKIE",
        "HTTP_USER_AGENT",
        "QUERY_STRING",
        "REQUEST_METHOD",
        "SCRIPT_NAME",
        "PATH_INFO",
        "SERVER_PROTOCOL"
    };
    int i = 0;

    while (!requestHeaderList[i].empty()) {
        const std::string&  value = request.findValue(requestHeaderList[i]);
        if (!value.empty()) {
            envs[envHeaderNames[i]] = value;
        }
        i++;
    }
    
    const std::string&  query = request.getQuery();
    if (!query.empty())
        envs["QUERY_STRING"] = query;
    envs["REQUEST_METHOD"] = request.getStrMethod();
    envs["PATH_INFO"] = request.getPath();
    envs["SERVER_PROTOCOL"] = request.getVersion();
// std::cout << "version: " << request.getVersion() << std::endl;
    //SEVER_NAME
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

bool Router::isHeaderEnd() {
	return request.isHeaderEnd();
}

bool Router::isRequestEnd() {
	return request.isRequestEnd();
}

bool Router::getHaveResponse(void) const {
	return haveResponse;
}

const std::string& Router::getResponse(void) const {
	return response.getResponseStr();
}

void Router::addRequest(const std::string &request) {
	this->request.addRequest(request);
}

void Router::parseHeader(void) {
	request.parseHeader();
}

void Router::parseBody(void) {
	request.parseBody();
}

void Router::setResponse(const std::string &src) {
	response.setResponse(src);
}

void Router::readCGI(void) {
	response.readCGI();
}

void Router::writeCGI(const intptr_t fdBufferSize) {
	response.writeCGI(fdBufferSize);
}

void Router::disconnectCGI(void) {
	response.disconnectCGI();
	haveResponse = true;
}

int Router::getWriteFd(void) const {
	return response.getWriteFd();
}

int Router::getReadFd(void) const {
	return response.getReadFd();
}

int Router::getRequestError() const {
    return request.getError();
}