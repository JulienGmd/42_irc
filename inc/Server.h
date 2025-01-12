#pragma once

#include "Client.h"

#include <map>
#include <netinet/in.h>
#include <set>
#include <string>
#include <vector>

class Server
{
public:
	Server();
	~Server();

private:
	int server_fd;
	sockaddr_in address;
	std::vector<Client> clients;

private:
	void start();
	void loop();
	void connect_clients(fd_set &read_fds);
	void handle_messages(fd_set &read_fds);
	void parse_command(const std::string &message, std::string &command, std::string &params);
	void set_non_blocking(int fd);
	void shutdown();
};
