#include "server.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>

int main()
{
	try
	{
		Server server;
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << ": " << std::strerror(errno) << std::endl;
	}
	catch (...)
	{
		std::cerr << "Unknown error occurred" << std::endl;
	}

	return 0;
}
