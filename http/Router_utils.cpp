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
    response.makeStatusLine("HTTP/1.1", "404", "Not Found");
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
        } else if (input[i] == '+') { // Handling the space encoded as '+'
            oss << ' ';
        } else {
            oss << input[i];
        }
    }
    return oss.str();
}

void Router::validateContentType() {
	std::string contentType = request.getHeaderValue("Content-Type");
	if (contentType != "application/x-www-form-urlencoded") {
		response.makeStatusLine("HTTP/1.1", "415", "Unsupported Media Type");
		throw std::runtime_error("Unsupported Media Type");
	}
}

void Router::parsePostData(std::string& title, std::string& postContent) {
	std::string content = request.getBody();
	std::stringstream ss(content);
	std::string key;
	std::string value;

	while (std::getline(ss, key, '=')) {
		std::getline(ss, value, '&');
		if (key == "title") title = URLDecode(value);
		if (key == "content") postContent = URLDecode(value);
	}
}

void Router::appendPostToFile(const std::string& title, const std::string& postContent) {
	std::ofstream outFile(post_txt, std::ios::app);
	if (!outFile) {
		std::cerr << "Could not open or create posts.txt" << std::endl;
        response.makeStatusLine("HTTP/1.1", "500", "Internal Server Error");
		throw std::runtime_error("File error");
	}
	outFile << "Title: " << title << "\nContent: " << postContent << "\n\n";
	outFile.close();
}

void Router::readAndModifyHTML(std::string& htmlResponse) {
    std::ifstream inFile(index_html);
    if (!inFile.is_open()) {
        std::cerr << "Error opening " << index_html << std::endl;
        response.makeStatusLine("HTTP/1.1", "500", "Internal Server Error");
        throw std::runtime_error("File open error");
    }
    htmlResponse.assign((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    std::size_t commentPos = htmlResponse.find("<!-- You can add forums, threads, posts, etc. here -->");
    if (commentPos != std::string::npos) {
        std::string postsHtml = readPosts();
        htmlResponse.replace(commentPos, std::string("<!-- You can add forums, threads, posts, etc. here -->").length(), postsHtml);
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
