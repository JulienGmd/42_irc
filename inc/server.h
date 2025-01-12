#pragma once

#include <netinet/in.h>
#include <vector>

class Server
{
public:
	Server();
	~Server();

private:
	int server_fd;
	sockaddr_in address;
	std::vector<int> clients;

private:
	void start();
	void loop();
	void connect_clients(fd_set &read_fds);
	void handle_messages(fd_set &read_fds);
	void set_non_blocking(int fd);
	void shutdown();
};
