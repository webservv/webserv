#include "Response.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <sys/_types/_pid_t.h>
#include <sys/_types/_ssize_t.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "Router.hpp"
#define CHILD_PID 0
#define READ 0
#define WRITE 1
#define BUFFER_SIZE 1000000
#define NULL_FD -1
#define NULL_PID -1

Response::Response():
	response(),
	messageToCGI(),
	writtenCgiLength(0),
	sentLenghth(0),
	writeFD(NULL_FD),
	readFD(NULL_FD),
	cgiPid(NULL_PID) {}

Response::Response(const Response& copy):
	response(copy.response),
	messageToCGI(copy.messageToCGI),
	writtenCgiLength(copy.writtenCgiLength),
	sentLenghth(copy.writtenCgiLength),
	writeFD(copy.writeFD),
	readFD(copy.readFD),
	cgiPid(copy.cgiPid) {}

Response& Response::operator=(const Response& copy) {
	response = copy.response;
	messageToCGI = copy.messageToCGI;
	writtenCgiLength = copy.writtenCgiLength;
	sentLenghth = copy.sentLenghth;
	writeFD = copy.writeFD;
	readFD = copy.readFD;
	cgiPid = copy.cgiPid;
	return *this;
}

Response::~Response() {}

void Response::bootCGI(int readPipe[2], int writePipe[2] ,std::map<std::string, std::string>& envs) const {
	char**	envList = makeEnvList(envs);
// int i = 0;
// while (envList[i]) {
// 	std::cout << envList[i] << std::endl;
// 	i++;
// }
	if (dup2(readPipe[WRITE], STDOUT_FILENO) < 0)
		throw Router::ErrorException(500, "processCGI: " + std::string(strerror(errno)));
	if (dup2(writePipe[READ], STDIN_FILENO) < 0)
		throw Router::ErrorException(500, "processCGI: " + std::string(strerror(errno)));
	close(readPipe[READ]);
	close(readPipe[WRITE]);
	close(writePipe[READ]);
	close(writePipe[WRITE]);
	if (envs["SCRIPT_NAME"] == "/directory/youpi.bla") { // test only
		if (execve("./cgi/cgi_tester", NULL, envList) < 0)
		throw Router::ErrorException(500, "processCGI: " + std::string(strerror(errno)));
	}
	if (execve(('.' + envs["SCRIPT_NAME"]).c_str(), NULL, envList) < 0)
		throw Router::ErrorException(500, "processCGI: " + std::string(strerror(errno)));
	std::exit(EXIT_FAILURE);
}

char** Response::makeEnvList(std::map<std::string, std::string>& envs) const {
	char**	envList = new char*[envs.size() + 1];
	int		i = 0;

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

void Response::addResponse(const std::string& str) {
	response.insert(response.end(), str.begin(), str.end());
}

void Response::makeStatusLine(const std::string& version, const std::string& statusCode, const std::string& statusMessage) {
	addResponse(version);
	addResponse(" ");
	addResponse(statusCode);
	addResponse(" ");
	addResponse(statusMessage);
	addResponse("\r\n");
}

void Response::makeHeader(const std::string& key, const std::string& value) {
	addResponse(key);
	addResponse(": ");
	addResponse(value);
	addResponse("\r\n");
}

void Response::makeBody(const std::vector<char>& data, const size_t len, const std::string& type) {
	std::stringstream	ss;
	
	ss << len;
	addResponse("Content-Length: " + ss.str() + "\r\n");
	addResponse("Content-Type: " + type + "\r\n");
	addResponse("\r\n");
	response.insert(response.end(), data.begin(), data.end());
}

const std::vector<char>& Response::getResponse(void) const {
	return response;
}

int Response::getWriteFD(void) const {
	return writeFD;
}

int Response::getReadFD(void) const {
	return readFD;
}

size_t Response::getSentLength(void) const {
	return sentLenghth;
}

void Response::setSentLength(const size_t size) {
	sentLenghth = size;
}

void Response::setResponse(const std::vector<char> &src) {
	response = src;
}

void Response::setMessageToCGI(const std::vector<char> &src) {
	messageToCGI = src;
}

void Response::connectCGI(std::map<std::string, std::string>& envs) {
	int		readPipe[2];
	int		writePipe[2];

	if (envs["SCRIPT_NAME"] == "/directory/youpi.bla") { // test only
		if (access("./cgi/cgi_tester", F_OK))
		throw Router::ErrorException(500, "connectCGI0: " + std::string(strerror(errno)));
	}
	else if (access(('.' + envs["SCRIPT_NAME"]).c_str(), F_OK))
		throw Router::ErrorException(500, "connectCGI1: " + std::string(strerror(errno)));
	if (pipe(readPipe) < 0 || pipe(writePipe) < 0)
		throw Router::ErrorException(500, "connectCGI2: " + std::string(strerror(errno)));
	cgiPid = fork();
	if (cgiPid < 0)
		throw Router::ErrorException(500, "connectCGI3: " + std::string(strerror(errno)));
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
// std::cout << "readFromCGI" << std::endl;
	read_size = read(readFD, buf, BUFFER_SIZE);
	if (read_size < 0)
		throw Router::ErrorException(500, "readCGI: " + std::string(strerror(errno)));
	buf[read_size] = '\0';
	addResponse(buf);
}

void Response::writeToCGI(const intptr_t fdBufferSize) {
	const intptr_t	bufSize = fdBufferSize < BUFFER_SIZE ? fdBufferSize : BUFFER_SIZE;
	const size_t	leftSize = messageToCGI.size() - writtenCgiLength;
	ssize_t			writeLength;
// std::cout << "writeToCGI" << std::endl;
// std::cout << "leftSize: " << leftSize << std::endl;
	if (leftSize == 0) {
		close(writeFD);
		writeFD = NULL_FD;
		return;
	}
	if (leftSize <= static_cast<size_t>(bufSize)) {
		writeLength = write(writeFD, messageToCGI.data() + writtenCgiLength, leftSize);
		if (writeLength < 0)
			throw Router::ErrorException(500, "writeCGI: " + std::string(strerror(errno)));
	}
	else {
		writeLength = write(writeFD, messageToCGI.data() + writtenCgiLength, bufSize);
		if (writeLength < 0)
			throw Router::ErrorException(500, "writeCGI: " + std::string(strerror(errno))); 
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
			throw Router::ErrorException(500, "disconnectCGI1: " + std::string(strerror(errno)));
		if (!WIFEXITED(stat))
			throw Router::ErrorException(500, "disconnectCGI2: " + std::string(strerror(errno)));
		cgiPid = NULL_PID;
	}
}

void Response::endResponse(void) {
	addResponse("\r\n");
}