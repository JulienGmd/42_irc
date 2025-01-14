#pragma once

#include "Client.hpp"

#include <map>
#include <netinet/in.h>
#include <set>
#include <string>
#include <vector>

class Server
{
public:
	Server(int port, const std::string &password);
	~Server();

private:
	const int PORT;
	const std::string PASSWORD;
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
