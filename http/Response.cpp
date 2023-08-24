#include "Response.hpp"
#include <cstddef>
#include <cstring>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <sys/_types/_pid_t.h>
#include <sys/_types/_ssize_t.h>
#include <sys/wait.h>
#include <unistd.h>
#define CHILD_PID 0
#define READ 0
#define WRITE 1
#define BUFFER_SIZE 2048
#define NULL_FD -1
#define NULL_PID -1

static const std::string g_COOKIE_KEY = "Cookie=";

Response::Response():
	responseStr(""),
	messageToCGI(""),
	writeFd(NULL_FD),
	readFd(NULL_FD),
	cgiPid(NULL_PID) {}

Response::Response(const Response& copy):
	responseStr(copy.responseStr),
	messageToCGI(copy.messageToCGI),
	writeFd(copy.writeFd),
	readFd(copy.readFd),
	cgiPid(copy.cgiPid) {}

Response& Response::operator=(const Response& copy) {
	responseStr = copy.responseStr;
	messageToCGI = copy.messageToCGI;
	writeFd = copy.writeFd;
	readFd = copy.readFd;
	cgiPid = copy.cgiPid;
	return *this;
}

Response::~Response() {}

void Response::makeStatusLine(const std::string& version, const std::string& statusCode, const std::string& statusMessage) {
	responseStr += version + " " + statusCode + " " + statusMessage + "\r\n";
}

void Response::makeHeader(const std::string& key, const std::string& value) {
	responseStr += key + ": " + value + "\r\n";
}

void Response::makeBody(const std::string& data, const size_t len, const std::string& type) {
	std::stringstream	ss;
	
	ss << len;
	responseStr += "Content-Length: " + ss.str() + "\r\n";
	responseStr += "Content-Type: " + type + "\r\n";
	responseStr += "\r\n";
	responseStr += data;
}

const std::string& Response::getResponseStr(void) const {
	return responseStr;
}

int Response::getWriteFd(void) const {
	return writeFd;
}

int Response::getReadFd(void) const {
	return readFd;
}

void Response::setResponse(const std::string &src) {
	responseStr.assign(src);
}

void Response::setMessageToCGI(const std::string &src) {
	messageToCGI = src;
}

void Response::connectCGI(std::map<std::string, std::string>& envs) {
	int		readPipe[2];
	int		writePipe[2];

	if (access(('.' + envs["SCRIPT_NAME"]).c_str(), F_OK))
		throw std::runtime_error("getFromCGI1: " + std::string(strerror(errno)));
	if (pipe(readPipe) < 0 || pipe(writePipe) < 0)
		throw std::runtime_error("getFromCGI2: " + std::string(strerror(errno)));
	cgiPid = fork();
	if (cgiPid < 0)
		throw std::runtime_error("getFromCGI3: " + std::string(strerror(errno)));
	else if (cgiPid == CHILD_PID)
		processCGI(readPipe, envs);
	close(readPipe[WRITE]);
	close(writePipe[READ]);
	writeFd = writePipe[WRITE];
	readFd = readPipe[READ];
}

void Response::processCGI(int fd[2], std::map<std::string, std::string>& envs) {
	char**	envList = makeEnvList(envs);

	if (dup2(fd[WRITE], STDOUT_FILENO) < 0)
		throw std::runtime_error("processCGI: " + std::string(strerror(errno)));
	close(fd[READ]);
	close(fd[WRITE]);
	if (execve(('.' + envs["SCRIPT_NAME"]).c_str(), NULL, envList) < 0)
		throw std::runtime_error("processCGI: " + std::string(strerror(errno)));
}

char** Response::makeEnvList(std::map<std::string, std::string>& envs) const {
	char**	envList = new char*[envs.size() + 1];
	int				i = 0;

	for (std::map<std::string, std::string>::const_iterator it = envs.begin(); it != envs.end(); it++) {
		std::string	tmp = it->first + "=" + it->second;
		envList[i] = new char[tmp.size() + 1];
		for (size_t j = 0; j < tmp.size(); j++) {
			envList[i][j] = tmp[j];
		}
		envList[i][tmp.size()] = '\0';
		i++;
	}
	envList[i] = NULL;
	return envList;
}

void Response::readCGI(void) {
	ssize_t	read_size;
	char	buf[BUFFER_SIZE + 1];

	while (true) {
		read_size = read(readFd, buf, BUFFER_SIZE);
		if (read_size < 0)
			throw std::runtime_error("readCGI: " + std::string(strerror(errno)));
		else if (read_size == 0)
			break;
		buf[read_size] = '\0';
		responseStr += buf;
	}
}

void Response::writeCGI(const intptr_t fdBufferSize) {
	intptr_t	bufSize = fdBufferSize < BUFFER_SIZE ? fdBufferSize : BUFFER_SIZE;
	ssize_t		writeLength;
	if (messageToCGI.size() == 0) {
		close(writeFd);
		writeFd = NULL_FD;
		return;
	}
	if (static_cast<intptr_t>(messageToCGI.size()) <= bufSize) {
		writeLength = write(writeFd, messageToCGI.c_str(), messageToCGI.size());
		if (writeLength < 0)
			throw std::runtime_error("writeCGI: " + std::string(strerror(errno)));
		if (writeLength == static_cast<ssize_t>(messageToCGI.size())) {
			messageToCGI.clear();
			close(writeFd);
			writeFd = NULL_FD;
		}
		else
			messageToCGI = messageToCGI.substr(writeLength, -1);
	}
	else {
		const std::string writeMessage = messageToCGI.substr(0, bufSize);
		messageToCGI = messageToCGI.substr(bufSize, -1);
		if (write(writeFd, writeMessage.c_str(), writeMessage.size()) < 0)
			throw std::runtime_error("writeCGI: " + std::string(strerror(errno)));
	}
}

void Response::disconnectCGI(void) {
	int	stat;

	if (writeFd != NULL_FD)
		close(writeFd);
	if (readFd != NULL_FD)
		close(readFd);
	if (cgiPid != NULL_PID) {
		if (waitpid(cgiPid, &stat, 0) < 0)
			throw std::runtime_error("disconnectCGI: " + std::string(strerror(errno)));
		if (!WIFEXITED(stat))
			throw std::logic_error("disconnectCGI: " + std::string(strerror(errno)));
	}
}

void Response::setCookieValue(const std::string& value) {
    cookieValue = value;
}