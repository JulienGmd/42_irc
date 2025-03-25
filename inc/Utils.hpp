#pragma once

#include <string>

namespace Utils
{
	int str_to_int(const std::string &str);
	void parse_args(int ac, char **av, int &out_port, std::string &out_password);
	std::vector<std::string> split(const std::string &str, const std::string &delimiter);
	std::string chain_read(int fd);
}
