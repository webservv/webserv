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
        throw std::runtime_error("getFileType: " + std::string(strerror(errno)));
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

std::string Router::generateDirectoryListing(const std::string& directoryPath) {
    std::string html;

    html += "<html><head><title>Index of " + directoryPath + "</title></head><body>";
    html += "<h1>Index of " + directoryPath + "</h1><hr><pre>";
    html += makeSpacedStr(15, "fileName");
    html += makeSpacedStr(10, "fileType") + "fileSize" + "\n";

    DIR* dir = opendir(directoryPath.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            const std::string fileName = entry->d_name;
            if (fileName != "." && fileName != "..") {
                const std::string       filePath = directoryPath + "/" + fileName;
                const Router::eFileType fileType = getFileType(filePath.c_str());
                struct stat fileStat;

                if (stat(filePath.c_str(), &fileStat) == 0) {
                    const std::string fileSize = std::to_string(fileStat.st_size);

                    html += "<a href=\"" + fileName + "\">" + fileName + "</a>";
                    std::string tab(15 - fileName.length(), ' ');
                    html += tab + makeSpacedStr(10, std::to_string(fileType));
                    html += fileSize + "\n";
                }
                else
                    throw std::runtime_error("generateDirectoryListing: " + std::string(strerror(errno)));
            }
        }
        closedir(dir);
    }
    html += "</pre><hr></body></html>";

	std::string autoFile = directoryPath + "/" + "autoindex.html";
    std::ofstream outputFile(autoFile);
    if (outputFile.is_open()) {
        outputFile << html;
        outputFile.close();
        std::cout << "HTML directory listing generated successfully." << std::endl;
    } else {
        throw std::runtime_error("generateDirectoryListing: " + std::string(strerror(errno)));
    }
    return autoFile;
}
