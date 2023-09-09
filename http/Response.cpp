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
#define NULL_FD -1
#define NULL_PID -1

static const size_t	BUFS = 120000000;

Response::Response():
	response(),
	messageFromCGI(),
	messageToCGI(NULL),
	writtenCgiLength(0),
	sentLenghth(0),
	writeFD(NULL_FD),
	readFD(NULL_FD),
	cgiPid(NULL_PID) {
		response.reserve(BUFS);
		messageFromCGI.reserve(BUFS);
	}

Response::Response(const Response& copy):
	response(copy.response),
	messageFromCGI(),
	messageToCGI(copy.messageToCGI),
	writtenCgiLength(copy.writtenCgiLength),
	sentLenghth(copy.writtenCgiLength),
	writeFD(copy.writeFD),
	readFD(copy.readFD),
	cgiPid(copy.cgiPid) {
		response.reserve(BUFS);
		messageFromCGI.reserve(BUFS);
	}

Response& Response::operator=(const Response& copy) {
	response = copy.response;
	messageFromCGI = copy.messageFromCGI;
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
	if (envs["SCRIPT_NAME"].rfind(".bla") != std::string::npos) { // test only
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

void Response::addResponse(std::vector<char>& src, const std::string& str) {
	src.insert(src.end(), str.begin(), str.end());
}

void Response::checkCgiResponse(void) {
	std::string header;
	size_t		bodyPos = messageFromCGI.size();

	for (size_t i = 0; i < messageFromCGI.size() - 3; ++i) {
		if (messageFromCGI[i] == '\r' && messageFromCGI[i + 1] == '\n' && messageFromCGI[i + 2] == '\r' && messageFromCGI[i + 3] == '\n') {
			bodyPos = i + 4;
			header.assign(messageFromCGI.begin(), messageFromCGI.begin() + i);
			if (findIgnoreCase(header, "content-length") != std::string::npos) {
				return;
			}
			break;
		}
	}
	if (bodyPos >= messageFromCGI.size())
		return;
	addCgiContentLength(messageFromCGI.size() - bodyPos);
}
//WIP
void Response::addCgiContentLength(const size_t size) {
	std::string			contentLength = "Content-Length";
	std::string			strSize;
	std::stringstream	ss;
	std::vector<char>	newResponse;	

	ss << size;
	ss >> strSize;
	contentLength += ": " + strSize + "\r\n";
	newResponse.reserve(contentLength.size() + messageFromCGI.size());
	newResponse.insert(newResponse.end(), contentLength.begin(), contentLength.end());
	newResponse.insert(newResponse.end(), messageFromCGI.begin(), messageFromCGI.end());
	messageFromCGI.swap(newResponse);
}

size_t Response::findIgnoreCase(const std::string& src, const std::string& find) const {
	char	srcLower;
	char	findLower;
	size_t	matchedSize;

	for (size_t i = 0; i < src.size() - (find.size() - 1); ++i) {
		matchedSize = 0;
		for (size_t j = 0; j < find.size(); ++j) {
			srcLower = (src[i + j] >= 'A' && src[i + j] <= 'Z') ? src[i + j] - 'A' : src[i + j];
			findLower = (find[j] >= 'A' && find[j] <= 'Z') ? find[j] - 'A' : find[j];
			if (srcLower == findLower)
				++matchedSize;
		}
		if (matchedSize == find.size()) {
			return i;
		}
	}
	return std::string::npos;
}

void Response::makeStatusLine(const std::string& version, const std::string& statusCode, const std::string& statusMessage) {
	addResponse(response, version);
	addResponse(response, " ");
	addResponse(response, statusCode);
	addResponse(response, " ");
	addResponse(response, statusMessage);
	addResponse(response, "\r\n");
}

void Response::makeHeader(const std::string& key, const std::string& value) {
	addResponse(response, key);
	addResponse(response, ": ");
	addResponse(response, value);
	addResponse(response, "\r\n");
}

void Response::makeBody(const std::vector<char>& data, const size_t len, const std::string& type) {
	std::stringstream	ss;
	
	ss << len;
	addResponse(response, "Content-Length: " + ss.str() + "\r\n");
	addResponse(response, "Content-Type: " + type + "\r\n");
	addResponse(response, "\r\n");
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
	messageToCGI = &src;
}

void Response::connectCGI(std::map<std::string, std::string>& envs) {
	int		readPipe[2];
	int		writePipe[2];

	if (envs["SCRIPT_NAME"].rfind(".bla") != std::string::npos) { // test only
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
	static const size_t BUFFER_SIZE = 1000000;
	ssize_t				readSize;
	std::vector<char>	buf(BUFFER_SIZE);

	readSize = read(readFD, buf.data(), BUFFER_SIZE);
	if (readSize < 0)
		throw Router::ErrorException(500, "readCGI: " + std::string(strerror(errno)));
	buf.resize(readSize);
	response.insert(response.end(), buf.begin(), buf.end());
}

void Response::writeToCGI(const intptr_t fdBufferSize) {
	const size_t		leftSize = messageToCGI->size() - writtenCgiLength;
	const size_t		bufferSize = leftSize <= static_cast<size_t>(fdBufferSize) ? leftSize : fdBufferSize;
	ssize_t				writeLength;

	if (leftSize == 0) {
		close(writeFD);
		writeFD = NULL_FD;
		return;
	}
	writeLength = write(writeFD, messageToCGI->data() + writtenCgiLength, bufferSize);
	if (writeLength < 0)
		throw Router::ErrorException(500, "writeCGI: " + std::string(strerror(errno))); 
	if (writeLength + writtenCgiLength == messageToCGI->size()) {
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
	// checkCgiResponse();
	// response.insert(response.end(), messageFromCGI.begin(), messageFromCGI.end());
// const size_t	size = response.size() < 500 ? response.size() : 500;
// for (size_t i = 0; i < size; i++) {
// 	std::cout << response[i];
// }
// std::cout << "\n" << std::endl;
}

void Response::endResponse(void) {
	addResponse(response, "\r\n");
}