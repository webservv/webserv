#include "Server.hpp"
#define NULL_FD -1
#define EVENTS_SIZE 10000

Server* Server::instance = NULL;
static void listenSocket(int socket_fd);
static void bindSocket(const int port, int socket_fd);
static in_addr_t IPToInt(const std::string& ip);
static void setSocketOptions(int socket_fd);

Server::Server():
    kqueueFd(NULL_FD),
    listenSockets(),
	IOchanges(),
	IOevents(EVENTS_SIZE),
	clientSockets(),
	pipes(),
    cookies(),
    configs() {}

Server::Server(const Config& config):
    kqueueFd(NULL_FD),
    listenSockets(),
    IOchanges(),
    IOevents(EVENTS_SIZE),
    clientSockets(),
    pipes(),
    cookies(),
    configs()
{
    kqueueFd = kqueue();
    if (kqueueFd < 0) {
        throw std::runtime_error("kqueue error: " + std::string(strerror(errno)));
    }

    const std::vector<ServerConfig>& servers = config.getServers();
    for (std::vector<ServerConfig>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
        const ServerConfig& new_server = *it;

        int new_socket_fd = createSocket();
        setSocketOptions(new_socket_fd);
        bindSocket(new_server.getListenPort(), new_socket_fd);
        listenSocket(new_socket_fd);
        listenSockets[new_socket_fd] = new_socket_fd;
        configs[new_socket_fd] = &new_server;
        std::cout << "Server started on " << new_server.getServerName() << ":"
                  << new_server.getListenPort() << ", waiting for connections..." << std::endl;
    }
}

Server::Server(const Server& copy) { static_cast<void>(copy); }

Server& Server::operator=(const Server& copy) {
	static_cast<void>(copy);
	return *this;
}

Server::~Server() {
    for (std::map<int, Router>::iterator it = clientSockets.begin(); it != clientSockets.end(); ++it) {
        disconnect(it->first);
    }
    for (std::map<int, int>::iterator it = listenSockets.begin(); it != listenSockets.end(); ++it) {
        close(it->second);
    }
    close(kqueueFd);
}

int Server::createSocket() {
    int new_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (new_socket_fd < 0) {
        throw std::runtime_error("ERROR opening socket");
    }

    if (fcntl(new_socket_fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC) < 0) {
        throw std::runtime_error("fcntl error! " + std::string(strerror(errno)));
    }
    AddIOReadChange(new_socket_fd);
    return new_socket_fd;
}


Server& Server::getInstance(const Config& config) {
	if (instance == NULL) {
		instance = new Server(config);
	}
	return *instance;
}

void Server::addPipes(const int writeFd, const int readFd, Router* const router) {
	pipes[writeFd] = router;
	pipes[readFd] = router;
    AddIOReadChange(readFd);
    AddIOWriteChange(writeFd);
}

void Server::addCookie(const std::string& key, const std::string& value) {
    cookies[key] = value;
}

const std::string& Server::getCookie(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = cookies.find(key);
    return it->second;
}

static void listenSocket(int socket_fd) {
    int backlog = 1000;

	if (listen(socket_fd, backlog) < 0) {
		throw std::runtime_error("ERROR on listening");
	}
}

static void bindSocket(const int port, int socket_fd) {
    sockaddr_in server_addr;
    const std::string& host = "127.0.0.1";

    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = IPToInt(host);
    
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        throw std::runtime_error("ERROR invalid host");
    }

    if (bind(socket_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        throw std::runtime_error("ERROR on binding: " + std::string(strerror(errno)));
    }
}

static in_addr_t IPToInt(const std::string& ip) {
	in_addr_t	ret = 0;
	int			tmp = 0;
	int			bitShift = 0;
	
	for (size_t i = 0; i < ip.size(); i++) {
		if (ip[i] == '.') {
			ret += tmp << bitShift;
			tmp = 0;
			bitShift += 8;
			continue;
		}
		tmp = tmp * 10 + ip[i] - '0';
	}
	ret += tmp << bitShift;
	return ret;
}

static void setSocketOptions(int socket_fd) {
    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("ERROR setting socket options");
    }
}