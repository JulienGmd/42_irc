#include "_config.hpp"
#include <errno.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <vector>

namespace Utils
{
	int str_to_int(const std::string &str)
	{
		std::stringstream ss(str);
		int num;
		ss >> num;
		if (ss.fail())
			throw std::runtime_error("Invalid number: " + str);
		return num;
	}

	void parse_args(int ac, char **av, int &out_port, std::string &out_password)
	{
		if (ac != 3)
			throw std::runtime_error("Usage: " + std::string(av[0]) + " <port> <password>");
		try
		{
			out_port = str_to_int(av[1]);
			out_password = av[2];
		}
		catch (const std::exception &e)
		{
			std::string err_msg = e.what();
			if (err_msg.find("Invalid number") != std::string::npos)
				throw std::runtime_error("Invalid port number");
			throw std::runtime_error("Parsing error");
		}
	}

	std::vector<std::string> split(const std::string &str, const std::string &delimiter)
	{
		std::vector<std::string> result;
		std::stringstream ss(str);
		std::string item;

		while (std::getline(ss, item, delimiter[0]))
			result.push_back(item);

		return result;
	}

	std::string chain_read(int fd)
	{
		std::string result;
		char buffer[BUFFER_SIZE];
		int bytes_read;

		while ((bytes_read = recv(fd, buffer, BUFFER_SIZE - 1, 0)) > 0)
		{
			buffer[bytes_read] = '\0';
			result += buffer;
		}

		if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
			throw std::runtime_error("chain_read: recv failed");

		return result;
	}
}
