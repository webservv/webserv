#include <cstring>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>
#include "Router.hpp"



Router::eFileType Router::getFileType(const char* path) const {
    struct stat info;

    if (stat(path, &info) != 0) {
        throw Router::ErrorException(500, "getFileType: " + std::string(strerror(errno)));
    }
    if (S_ISDIR(info.st_mode)) {
        return dir;
    } else if (S_ISREG(info.st_mode)) {
        return file;
    } else {
        return other;
    }
}

const std::string Router::makeSpacedStr(const int desiredSpaces, std::string target) const {
    for (int i = 0; i < desiredSpaces; ++i) {
        target.push_back(' ');
    }
    return target;
}

std::string Router::generateDirectoryListing(const std::string& directoryPath, const std::string& URLPath) {
    std::string html;
    struct dirent* entry;
    DIR* dir = opendir(('.' + directoryPath).c_str());

    if (dir == NULL)
        throw Router::ErrorException(500, "generateDirectoryListing: opendir failure");
    html += "<html><head><title>Index of " + directoryPath + "</title></head><body>";
    html += "<h1>Index of " + directoryPath + "</h1><hr><pre>";
    html += makeSpacedStr(15, "fileName");
    html += makeSpacedStr(10, "fileType") + "fileSize" + "\n";
    entry = readdir(dir);
    while (entry != NULL) {
        std::string fileName = entry->d_name;
        if (fileName != ".") {
            const std::string       filePath =  '.' + directoryPath + "/" + fileName;
            const Router::eFileType fileType = getFileType(filePath.c_str());
            struct stat fileStat;
            if (stat(filePath.c_str(), &fileStat) == 0) {
                const std::string fileSize = std::to_string(fileStat.st_size);
                if (fileName.length() > 20)
                    fileName.erase(20);
                if (fileName == "..") {
                    html += "<a href=\"" + URLPath + "/" + fileName + "\">" + "[parent directory]" + "</a>";
                    std::string tab = "  ";
                    html += tab + makeSpacedStr(10, std::to_string(fileType));
                }
                else {
                    html += "<a href=\"" + URLPath + "/" + fileName + "\">" + fileName + "</a>";
                    std::string tab(20 - fileName.length(), ' ');
                    html += tab + makeSpacedStr(10, std::to_string(fileType));
                }
                html += fileSize + "\n";
            }
            else
                throw Router::ErrorException(500, "generateDirectoryListing: " + std::string(strerror(errno)));
        }
        entry = readdir(dir);
    }
    closedir(dir);
    html += "</pre><hr></body></html>";

    std::string  autoFile = '.' + directoryPath + "/" + "autoindex.html";
    std::ofstream       outputFile(autoFile);
    if (outputFile.is_open()) {
        outputFile << html;
        outputFile.close();
        std::cout << "[autoindex] HTML directory listing generated successfully." << std::endl;
    } else {
        throw Router::ErrorException(500, "generateDirectoryListing: " + std::string(strerror(errno)));
    }
    autoFile.erase(0, 1);
    return autoFile;
}
