#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>

std::string getFileType(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return "-";
    }

    if (S_ISDIR(info.st_mode)) {
        return "dir";
    } else if (S_ISREG(info.st_mode)) {
        return "file";
    } else {
        return "other";
    }
}

std::string generateDirectoryListing(const std::string& directoryPath) {
    std::string html;

    html += "<html><head><title>Index of " + directoryPath + "</title></head><body>";
    html += "<h1>Index of " + directoryPath + "</h1><hr><pre>";

    DIR* dir = opendir(directoryPath.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            const std::string fileName = entry->d_name;
            if (fileName != "." && fileName != "..") {
                const std::string filePath = directoryPath + "/" + fileName;
                const std::string fileType = getFileType(filePath.c_str());
                const std::string fileSize = (fileType == "dir") ? "-" : std::to_string(entry->d_reclen);
                const std::string lastModified = (fileType == "dir") ? "-" : std::to_string(entry->d_mtime);

                html += "<a href=\"" + fileName + "\">" + fileName + "</a>";
                html += "                 " + lastModified + "   " + fileSize + "\n";
            }
        }
        closedir(dir);
    }

    html += "</pre><hr></body></html>";

    return html;
}