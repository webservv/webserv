#ifndef RESPONSE_HPP
#define RESPONSE_HPP
#include <string>
#include <map>
#include <vector>

#include "Buffer.hpp"

class Response {
private:
	std::vector<char>			response;
	const std::vector<char>*	messageToCGI;
	size_t						writtenCgiLength;
	size_t						sentLenghth;
	int							writeFD;
	int							readFD;
	pid_t						cgiPid;
//Response.cpp
public:
	Response();
	~Response();
	Response(const Response& copy);
	Response&	operator=(const Response& copy);
private:
	void	bootCGI(int readPipe[2], int writePipe[2], std::map<std::string, std::string>& envs) const;
	char**	makeEnvList(std::map<std::string, std::string>& envs) const;
	void	addResponse(std::vector<char>& src, const std::string& str);
	size_t	findIgnoreCase(const std::string& src, const std::string& find) const;
public:
	void						makeStatusLine(const std::string& version, const std::string& statusCode, const std::string& statusMessage);
	void						makeHeader(const std::string& key, const std::string& value);
	void						makeBody(const std::vector<char>& data, const size_t len, const std::string& type);
	const std::vector<char>&	getResponse(void) const;
	int							getWriteFD(void) const;
	int							getReadFD(void) const;
	size_t						getSentLength(void) const;
	void						setSentLength(const size_t size);
	void						setResponse(const std::vector<char>& src);
	void						setMessageToCGI(const std::vector<char>& src);
	void						connectCGI(std::map<std::string, std::string>& envs);
	void						readFromCGI(void);
	void						writeToCGI(const intptr_t fdBufferSize);
	void						disconnectCGI(void);
	void						endResponse(void);
	void						clear(void);
};

#endif
