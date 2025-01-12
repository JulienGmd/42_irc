#include "server.h"

#include <iostream>
#include <stdexcept>

int main()
{
	try
	{
		start_server();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Unknown error occurred" << std::endl;
	}

	return 0;
}
