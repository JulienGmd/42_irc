#pragma once

#include "Channel.hpp"
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
	std::vector<Channel> channels;

private:
	void start();
	void loop();
	void connect_client(fd_set &read_fds);
	std::vector<Client>::iterator disconnect_client(const std::vector<Client>::iterator &it);
	void handle_clients_messages(fd_set &read_fds);
	bool handle_client_messages(Client &client, const std::string &buffer, int bytes_read);
	void parse_command(const std::string &message, std::string &out_command, std::string &out_params);
	void set_non_blocking(int fd);
	void shutdown();
};
