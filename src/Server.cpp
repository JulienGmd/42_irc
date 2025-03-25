#include "Server.hpp"
#include "_config.hpp"
#include "channelCommands.hpp"
#include "serverCommands.hpp"
#include <Utils.hpp>

#include <cstring> // TODO C
#include <fcntl.h> // TODO C
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h> // TODO C
#include <vector>

Server::Server(int port, const std::string &password)
	: PORT(port), PASSWORD(password), server_fd(-1), address(), clients()
{
	Channel a("#general");
	a.changetopic("On parle de tout et de rien");
	a.changemode("+l");
	a.changeul(1024);
	this->channels.push_back(a);
	Channel b("#actu");
	b.changetopic("On parle d'actu ici");
	b.changemode("+l");
	b.changeul(1);
	this->channels.push_back(b);
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
	clients.push_back(Client(new_socket));
	// Retrieve the hostname of the client
	char host[NI_MAXHOST];
	if (getnameinfo((struct sockaddr *)&address, addrlen, host, NI_MAXHOST, NULL, 0, 0) != 0)
		strcpy(host, "UNKNOWN");
	clients.back().hostname = host;

	std::cout << "New connection: " << host << ", Socket: " << new_socket << std::endl;
	send(new_socket, "Welcome!\n", 9, 0);
}

std::vector<Client>::iterator Server::disconnect_client(const std::vector<Client>::iterator &it)
{
	// TODO disconnect from channel

	Client &client = *it;

	for (size_t i = 0; i < channels.size(); i++)
		if (channels[i].hasuser(client))
			part(&client, channels[i].getid(), channels);

	close(client.socket);
	std::cout << "Client disconnected: " << client.socket << std::endl;
	return clients.erase(it);
}

/**
 * Handle messages from clients.
 * @param read_fds Set of file descriptors that have pending data
 */
void Server::handle_clients_messages(fd_set &read_fds)
{
	for (std::vector<Client>::iterator it = clients.begin(); it != clients.end();)
	{
		Client &client = *it;
		if (!FD_ISSET(client.socket, &read_fds))
		{
			++it;
			continue;
		}

		char buffer[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);

		// TODO gerer si le message est plus grand que BUFFER_SIZE
		int bytes_read = read(client.socket, buffer, BUFFER_SIZE);
		client.buffer += buffer;

		if (bytes_read <= 0)
		{
			it = disconnect_client(it);
			continue;
		}

		if (buffer[bytes_read - 1] == '\n')
		{
			std::cout << "Message from client ("
					  << "Hostname: " << client.hostname
					  << ", Nickname: " << client.nickname
					  << ", Username: " << client.username
					  << ", Socket: " << client.socket << ")\n"
					  << client.buffer;

			if (!handle_client_messages(client, client.buffer))
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
 * @param buffer Message sent by the client.
 * @return False if the client should be disconnected, true otherwise.
 */
bool Server::handle_client_messages(Client &client, const std::string &buffer)
{
	std::vector<std::string> messages = Utils::split(buffer, "\n");
	for (size_t i = 0; i < messages.size(); i++)
	{
		std::string message = messages[i];
		if (message.empty())
			continue;

		std::string command, params;
		parse_command(message, command, params);

		if (handle_channel_command(&client, command, params, channels))
			continue;

		if (!handle_server_command(client, command, params, clients, channels, PASSWORD))
			return false;
	}
	return true;
}

void Server::parse_command(const std::string &message, std::string &out_command, std::string &out_params)
{
	size_t pos = message.find(" ");
	int len = message.size();
	if (len > 0 && message[len - 1] == '\r')
		len--;
	if (pos != std::string::npos)
	{
		out_command = message.substr(0, pos);
		out_params = message.substr(pos + 1, len - pos - 1);
	}
	else
		out_command = message;
}

void Server::set_non_blocking(int fd)
{
	// TODO utiliser que la 2eme ligne
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::shutdown()
{
	for (size_t i = 0; i < clients.size(); i++)
		close(clients[i].socket);
	close(server_fd);
}
