#include "Server.hpp"
#include "_config.hpp"
#include "channelCommands.hpp"
#include "serverCommands.hpp"
#include <Utils.hpp>

#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <vector>

bool ShouldStop = false;

void sigint_handler(int signum);

Server::Server(int port, const std::string &password)
	: PORT(port), PASSWORD(password), server_fd(-1), address(), clients(), channels()
{
	start();
	loop();
}

Server::~Server()
{
	std::cout << "\nStopping server..." << std::endl;

	close(server_fd);

	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		close(it->first);
}

/**
 * Start the server.
 */
void Server::start()
{
	signal(SIGINT, sigint_handler);

	// Create the server socket, which will listen for incoming connections.
	server_fd = socket(ADDRESS_FAMILY, SOCKET_TYPE, 0);
	if (server_fd < 0)
		throw std::runtime_error("Socket failed");

	// In case of server restart, this avoid "Address already in use" error.
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("Setsockopt failed");

	// Bind the socket to IP:port.
	address.sin_family = ADDRESS_FAMILY;
	address.sin_addr.s_addr = ADDRESS_IP;
	address.sin_port = htons(PORT);
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
		throw std::runtime_error("Bind failed");

	// Start listening for incoming connections.
	if (listen(server_fd, MAX_CONNECTING_CLIENTS) < 0)
		throw std::runtime_error("Listen failed");
	set_non_blocking(server_fd);
	std::cout << "IRC Server started on port " << PORT << std::endl;
}

/**
 * Main loop of the server.
 */
void Server::loop()
{
	fd_set read_fds;
	int max_fd = 0;

	while (true)
	{
		if (ShouldStop)
			break;

		// Add server_fd and client sockets to read_fds
		FD_ZERO(&read_fds);
		FD_SET(server_fd, &read_fds);
		max_fd = server_fd;

		for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		{
			FD_SET(it->first, &read_fds);
			max_fd = std::max(max_fd, it->first);
		}

		// Wait for activity on any of the sockets.
		// Set read_fds with sockets that have pending data,
		// server_fd will be set if there is a new connection,
		// client sockets will be set if there is a message from them.
		int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
		if (activity < 0)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("Select failed");
		}

		connect_client(read_fds);
		handle_clients_messages(read_fds);
	}
}

/**
 * Connect pending clients.
 * @param read_fds Set of file descriptors that have pending data
 */
void Server::connect_client(fd_set &read_fds)
{
	if (!FD_ISSET(server_fd, &read_fds))
		return;

	// Create a new socket for the client
	int new_socket;
	socklen_t addrlen = sizeof(address);
	new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
	if (new_socket < 0)
		throw std::runtime_error("Accept failed");

	// Add the client to the list of clients
	set_non_blocking(new_socket);
	clients[new_socket] = Client(new_socket);

	// Retrieve the hostname of the client
	char *ip_str = inet_ntoa(address.sin_addr);
	clients[new_socket].hostname = ip_str;

	std::cout << "Client connected: " << new_socket << std::endl;
}

std::map<int, Client>::iterator Server::disconnect_client(const std::map<int, Client>::iterator &it)
{
	Client &client = it->second;

	std::cout << "Client disconnected: " << client.socket << std::endl;
	for (size_t i = 0; i < channels.size(); i++)
		if (channels[i].hasuser(client))
			part(&client, channels[i].getid(), channels);

	close(client.socket);

	std::map<int, Client>::iterator next = it;
	++next;
	clients.erase(it);

	return next;
}

/**
 * Handle messages from clients.
 * @param read_fds Set of file descriptors that have pending data
 */
void Server::handle_clients_messages(fd_set &read_fds)
{
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end();)
	{
		Client &client = it->second;

		if (!FD_ISSET(client.socket, &read_fds))
		{
			++it;
			continue;
		}

		client.buffer += Utils::chain_read(client.socket);

		if (client.buffer.empty())
		{
			it = disconnect_client(it);
			continue;
		}

		if (client.buffer[client.buffer.size() - 1] == '\n')
		{
			std::cout << "Message from client ("
					  << "H=" << client.hostname
					  << ", N=" << client.nickname
					  << ", U=" << client.username
					  << ", S=" << client.socket << ")\n"
					  << client.buffer << std::endl;

			if (!handle_client_messages(client))
			{
				it = disconnect_client(it);
				continue;
			}
			client.buffer.clear();
		}

		++it;
	}
}

/**
 * Handle messages from a client.
 * @param client Client that sent the message.
 * @return False if the client should be disconnected, true otherwise.
 */
bool Server::handle_client_messages(Client &client)
{
	std::vector<std::string> messages = Utils::split(client.buffer, "\n");
	for (size_t i = 0; i < messages.size(); i++)
	{
		std::string message = messages[i];
		if (message.empty())
			continue;

		std::string command, params;
		parse_command(message, command, params);

		if (handle_channel_command(&client, command, params, channels, this))
			continue;

		if (!handle_server_command(client, command, params, clients, channels, PASSWORD))
			return false;
	}
	return true;
}

void Server::parse_command(const std::string &message, std::string &out_command, std::string &out_params)
{
	size_t pos;
	std::string msg = message;
	while ((pos = msg.find('\r')) != std::string::npos)
		msg.erase(pos, 1);

	pos = msg.find(" ");
	int len = msg.size();
	if (pos != std::string::npos)
	{
		out_command = msg.substr(0, pos);
		out_params = msg.substr(pos + 1, len - pos - 1);
	}
	else
		out_command = msg;
}

void Server::set_non_blocking(int fd)
{
	fcntl(fd, F_SETFL, O_NONBLOCK);
}

const std::map<int, Client> &Server::get_clients()
{
	return clients;
}

Channel *Server::add_channel(std::string name)
{
	channels.push_back(Channel(*this, name));
	return &channels.back();
}

void sigint_handler(int signum)
{
	(void)signum;
	ShouldStop = true;
}
