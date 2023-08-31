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

Response::Response():
	responseStr(""),
	messageToCGI(),
	writtenCgiLength(0),
	writeFD(NULL_FD),
	readFD(NULL_FD),
	cgiPid(NULL_PID) {}

Response::Response(const Response& copy):
	responseStr(copy.responseStr),
	messageToCGI(copy.messageToCGI),
	writtenCgiLength(copy.writtenCgiLength),
	writeFD(copy.writeFD),
	readFD(copy.readFD),
	cgiPid(copy.cgiPid) {}

Response& Response::operator=(const Response& copy) {
	responseStr = copy.responseStr;
	messageToCGI = copy.messageToCGI;
	writtenCgiLength = copy.writtenCgiLength;
	writeFD = copy.writeFD;
	readFD = copy.readFD;
	cgiPid = copy.cgiPid;
	return *this;
}

Response::~Response() {}

void Response::bootCGI(int readPipe[2], int writePipe[2] ,std::map<std::string, std::string>& envs) const {
	char**	envList = makeEnvList(envs);

	if (dup2(readPipe[WRITE], STDOUT_FILENO) < 0)
		throw std::runtime_error("processCGI: " + std::string(strerror(errno)));
	if (dup2(writePipe[READ], STDIN_FILENO) < 0)
		throw std::runtime_error("processCGI: " + std::string(strerror(errno)));
	close(readPipe[READ]);
	close(readPipe[WRITE]);
	close(writePipe[READ]);
	close(writePipe[WRITE]);
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

int Response::getWriteFD(void) const {
	return writeFD;
}

int Response::getReadFD(void) const {
	return readFD;
}

void Response::setResponse(const std::string &src) {
	responseStr.assign(src);
}

void Response::setMessageToCGI(const std::vector<char> &src) {
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
		bootCGI(readPipe, writePipe, envs);
	close(readPipe[WRITE]);
	close(writePipe[READ]);
	writeFD = writePipe[WRITE];
	readFD = readPipe[READ];
}

void Response::readFromCGI(void) {
	ssize_t	read_size;
	char	buf[BUFFER_SIZE + 1];

	while (true) {
		read_size = read(readFD, buf, BUFFER_SIZE);
		if (read_size < 0)
			throw std::runtime_error("readCGI: " + std::string(strerror(errno)));
		else if (read_size == 0)
			break;
		buf[read_size] = '\0';
		responseStr += buf;
	}
}

void Response::writeToCGI(const intptr_t fdBufferSize) {
	const intptr_t	bufSize = fdBufferSize < BUFFER_SIZE ? fdBufferSize : BUFFER_SIZE;
	const size_t	leftSize = messageToCGI.size() - writtenCgiLength;
	ssize_t			writeLength;

	if (leftSize == 0) {
		close(writeFD);
		writeFD = NULL_FD;
		return;
	}
	if (leftSize <= static_cast<size_t>(bufSize)) {
		writeLength = write(writeFD, messageToCGI.data() + writtenCgiLength, leftSize);
		if (writeLength < 0)
			throw std::runtime_error("writeCGI: " + std::string(strerror(errno)));
	}
	else {
		writeLength = write(writeFD, messageToCGI.data() + writtenCgiLength, bufSize);
		if (writeLength < 0)
			throw std::runtime_error("writeCGI: " + std::string(strerror(errno))); 
	}
	if (writeLength + writtenCgiLength == messageToCGI.size()) {
			close(writeFD);
			writeFD = NULL_FD;
	}
	else
		writtenCgiLength += writeLength;
}

void Response::disconnectCGI(void) {
	int	stat;

	if (writeFD != NULL_FD)
		close(writeFD);
	if (readFD != NULL_FD)
		close(readFD);
	if (cgiPid != NULL_PID) {
		if (waitpid(cgiPid, &stat, 0) < 0)
			throw std::runtime_error("disconnectCGI1: " + std::string(strerror(errno)));
		if (!WIFEXITED(stat))
			throw std::logic_error("disconnectCGI2: " + std::string(strerror(errno)));
		cgiPid = NULL_PID;
	}
}
