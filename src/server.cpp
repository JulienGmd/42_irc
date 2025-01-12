#include "_config.h"

#include "server.h"
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

void Server::start()
{
	// Create server socket: This is the entry point for clients
	server_fd = socket(ADDRESS_FAMILY, SOCKET_TYPE, 0);
	if (server_fd < 0)
		throw std::runtime_error("Socket failed");

	// Allow address reuse to avoid "Address already in use" error in case of server restart
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("Setsockopt failed");

	// Bind the socket to IP:port
	address.sin_family = ADDRESS_FAMILY;
	address.sin_addr.s_addr = ADDRESS_IP;
	address.sin_port = htons(PORT);
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
		throw std::runtime_error("Bind failed");

	// Start listening
	if (listen(server_fd, MAX_CONNECTING_CLIENTS) < 0)
		throw std::runtime_error("Listen failed");

	set_non_blocking(server_fd);
	std::cout << "IRC Server started on port " << PORT << std::endl;
}

void Server::loop()
{
	fd_set read_fds;

	while (true)
	{
		FD_ZERO(&read_fds);

		// Add sockets to set
		FD_SET(server_fd, &read_fds);
		int max_fd = server_fd;
		for (size_t i = 0; i < clients.size(); i++)
		{
			FD_SET(clients[i], &read_fds);
			if (clients[i] > max_fd)
				max_fd = clients[i];
		}

		// Wait for activity on any of the sockets
		// This will set read_fds with sockets that have pending data,
		// server_fd will be set if there is a new connection,
		// client sockets will be set if there is a message from them
		int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
		if (activity < 0)
			throw std::runtime_error("Select error");

		connect_clients(read_fds);
		handle_messages(read_fds);
	}
}

void Server::connect_clients(fd_set &read_fds)
{
	if (FD_ISSET(server_fd, &read_fds))
	{
		int new_socket;
		socklen_t addrlen = sizeof(address);
		new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
		if (new_socket >= 0)
		{
			set_non_blocking(new_socket);
			clients.push_back(new_socket);
			std::cout << "New connection: " << new_socket << std::endl;
		}
	}
}

void Server::handle_messages(fd_set &read_fds)
{
	for (std::vector<int>::iterator it = clients.begin(); it != clients.end();)
	{
		int client_fd = *it;
		if (FD_ISSET(client_fd, &read_fds))
		{
			char buffer[BUFFER_SIZE];
			memset(buffer, 0, BUFFER_SIZE);

			// TODO gerer si le message est plus grand que BUFFER_SIZE
			int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
			if (bytes_read <= 0)
			{
				// Client disconnected
				std::cout << "Client disconnected: " << client_fd << std::endl;
				close(client_fd);
				it = clients.erase(it);
			}
			else
			{
				// Broadcast message
				std::cout << "Message from client " << client_fd << ": " << buffer;
				for (size_t i = 0; i < clients.size(); i++)
					if (clients[i] != client_fd)
						send(clients[i], buffer, bytes_read, 0);
				++it;
			}
		}
		else
			++it;
	}
}

void Server::set_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::shutdown()
{
	for (size_t i = 0; i < clients.size(); i++)
		close(clients[i]);
	close(server_fd);
}
