#pragma once

#include <string>

class Client
{
public:
	Client();
	Client(int socket);
	bool is_connected();

public:
	int socket;
	bool has_set_server_password;
	std::string nickname;
	std::string username;
	std::string hostname;
	std::string buffer;
};
