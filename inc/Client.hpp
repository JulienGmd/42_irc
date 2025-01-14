#pragma once

#include <string>

struct Client
{
public:
	Client(int socket);

public:
	int socket;
	std::string nickname;
	std::string username;
	std::string hostname;
	std::string channel;
};
