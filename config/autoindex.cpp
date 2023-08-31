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

std::string desiredStr(const int desiredSpaces, const std::string& originStr) {

    std::string newStr = originStr;
    std::string spaces(desiredSpaces - originStr.length(), ' ');
    newStr += spaces;
    return newStr;
}

std::string generateDirectoryListing(const std::string& directoryPath) {
    std::string html;

    html += "<html><head><title>Index of " + directoryPath + "</title></head><body>";
    html += "<h1>Index of " + directoryPath + "</h1><hr><pre>";

    DIR* dir = opendir(directoryPath.c_str());
    html += desiredStr(15, "fileName");
    html += desiredStr(10, "fileType") + "fileSize" + "\n";
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            const std::string fileName = entry->d_name;
            if (fileName != "." && fileName != "..") {
                const std::string filePath = directoryPath + "/" + fileName;
                const std::string fileType = getFileType(filePath.c_str());
                
                struct stat fileStat;
                if (stat(filePath.c_str(), &fileStat) == 0) {
                    const std::string fileSize = (fileType == "dir") ? "-" : std::to_string(fileStat.st_size);

                    html += "<a href=\"" + fileName + "\">" + fileName + "</a>";
                    std::string tab(15 - fileName.length(), ' ');
                    html += tab + desiredStr(10, fileType);
                    html += fileSize + "\n";
                }
            }
        }
        closedir(dir);
    }

    html += "</pre><hr></body></html>";

    return html;
}

/*
read this usage!
int main() {
    std::string directoryPath = "/Users/yoonsele/project/webserv"; // Replace with your directory path
    std::string htmlContent = generateDirectoryListing(directoryPath);

    std::ofstream outputFile("directory_listing.html");
    if (outputFile.is_open()) {
        outputFile << htmlContent;
        outputFile.close();
        std::cout << "HTML directory listing generated successfully." << std::endl;
    } else {
        std::cerr << "Failed to open output file." << std::endl;
    }

    return 0;
}
*/
