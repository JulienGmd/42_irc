#pragma once

#include <string>

struct Client
{
public:
	Client(int socket);

public:
	int socket;
	std::string nickname;
	std::string channel;
};
