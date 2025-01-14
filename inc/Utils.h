#pragma once

#include <string>

namespace Utils
{
	int str_to_int(const std::string &str);
	void parse_args(int ac, char **av, int &out_port, std::string &out_password);
}
