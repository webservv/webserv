#include "Router.hpp"
#include <unistd.h>
#include <fstream>
#include <iostream>

static const std::string	g_dir = "./document";


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
