#include "Server.hpp"
#include "Utils.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

int main(int ac, char **av)
{
	try
	{
		int port;
		std::string password;
		Utils::parse_args(ac, av, port, password);
		Server server(port, password);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << ": " << std::strerror(errno) << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "Unknown error occurred" << std::endl;
		return 1;
	}

	return 0;
}
