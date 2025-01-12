#include "_config.h"

#include "Server.h"
#include <cstring> // TODO C
#include <fcntl.h> // TODO C
#include <iostream>
#include <unistd.h> // TODO C
#include <vector>

Server::Server()
	: server_fd(-1), address(), clients()
{
	start();
	loop();
}

Server::~Server()
{
	shutdown();
}

/**
 * Start the server.
 */
void Server::start()
{
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
		// Add server_fd and client sockets to read_fds
		FD_ZERO(&read_fds);
		FD_SET(server_fd, &read_fds);
		max_fd = server_fd;
		for (size_t i = 0; i < clients.size(); i++)
		{
			FD_SET(clients[i].socket, &read_fds);
			if (clients[i].socket > max_fd)
				max_fd = clients[i].socket;
		}

		// Wait for activity on any of the sockets.
		// Set read_fds with sockets that have pending data,
		// server_fd will be set if there is a new connection,
		// client sockets will be set if there is a message from them.
		int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
		if (activity < 0)
			throw std::runtime_error("Select failed");

		connect_clients(read_fds);
		handle_messages(read_fds);
	}
}

/**
 * Connect pending clients.
 * @param read_fds Set of file descriptors that have pending data
 */
void Server::connect_clients(fd_set &read_fds)
{
	if (FD_ISSET(server_fd, &read_fds))
	{
		// Create a new socket for the client
		int new_socket;
		socklen_t addrlen = sizeof(address);
		new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
		if (new_socket >= 0)
		{
			// Add the client to the list of clients
			set_non_blocking(new_socket);
			clients.push_back(Client(new_socket));
			std::cout << "New connection: " << new_socket << std::endl;
			send(new_socket, "Welcome!\n", 9, 0);
		}
	}
}

/**
 * Handle messages from clients.
 * @param read_fds Set of file descriptors that have pending data
 */
void Server::handle_messages(fd_set &read_fds)
{
	for (std::vector<Client>::iterator it = clients.begin(); it != clients.end();)
	{
		Client client = *it;
		if (FD_ISSET(client.socket, &read_fds))
		{

			char buffer[BUFFER_SIZE];
			memset(buffer, 0, BUFFER_SIZE);

			// TODO gerer si le message est plus grand que BUFFER_SIZE
			int bytes_read = read(client.socket, buffer, BUFFER_SIZE);
			std::cout << "Message from client " << client.socket << ": " << buffer;

			if (bytes_read <= 0)
			{
				// Client disconnected
				std::cout << "Client disconnected: " << client.socket << std::endl;
				// TODO disconnect from channel
				close(client.socket);
				it = clients.erase(it);
				continue;
			}

			std::string command, params;
			parse_command(buffer, command, params);
			std::cout << "Command: " << command << ", Params: " << params << std::endl;

			if (command == "NICK")
				client.nickname = params;
			else if (command == "JOIN")
			{
				std::string join_message = ":" + client.nickname + " JOIN " + params + "\n";
				send(client.socket, join_message.c_str(), join_message.size(), 0);
				// TODO add client to channel
			}
			else if (command == "PART")
			{
				// TODO disconnect client from channel
			}
			else if (command == "LIST")
			{
				// TODO list channels
			}
			else if (command == "PRIVMSG")
			{
				// TODO send message to user or channel
			}
			else if (command == "QUIT")
			{
				// TODO disconnect from channel
				close(client.socket);
				it = clients.erase(it);
				continue;
			}
			else
			{
				// TODO ? Send message to channel
				// for (size_t i = 0; i < clients.size(); i++)
				// 	if (clients[i].socket != client.socket)
				// 		send(clients[i].socket, buffer, bytes_read, 0);
			}

			++it;
		}
		else
			++it;
	}
}

void Server::parse_command(const std::string &message, std::string &command, std::string &params)
{
	size_t pos = message.find(" ");
	if (pos != std::string::npos)
	{
		command = message.substr(0, pos);
		params = message.substr(pos + 1);
	}
	else
		command = message;
}

void Server::set_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::shutdown()
{
	for (size_t i = 0; i < clients.size(); i++)
		close(clients[i].socket);
	close(server_fd);
}
