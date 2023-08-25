#ifndef RESPONSE_HPP
#define RESPONSE_HPP
#include <string>
#include <map>

class Response {
private:
	std::string	responseStr;
	std::string	messageToCGI;
	int			writeFd;
	int			readFd;
	pid_t		cgiPid;
    std::string cookieValue;
public:
	Response();
	~Response();
	Response(const Response& copy);
	Response&	operator=(const Response& copy);
private:
	void				processCGI(int readPipe[2], int writePipe[2], std::map<std::string, std::string>& envs);
	char**				makeEnvList(std::map<std::string, std::string>& envs) const;
public:
	void				makeStatusLine(const std::string& version, const std::string& statusCode, const std::string& statusMessage);
	void				makeHeader(const std::string& key, const std::string& value);
	void				makeBody(const std::string& data, const size_t len, const std::string& type);
	const std::string&	getResponseStr(void) const;
	int					getWriteFd(void) const;
	int					getReadFd(void) const;
	void				setResponse(const std::string& src);
	void				setMessageToCGI(const std::string& src);
	void				connectCGI(std::map<std::string, std::string>& envs);
	void				readCGI(void);
	void				writeCGI(const intptr_t fdBufferSize);
	void				disconnectCGI(void);
};

#endif
