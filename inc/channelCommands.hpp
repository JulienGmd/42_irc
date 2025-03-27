#pragma once

#include "Client.hpp"
#include "Server.hpp"

std::vector<std::string> splitString(const std::string &str, char delimiter);
bool handle_channel_command(Client *usr, std::string command, std::string params, std::vector<Channel> &channels, Server *server);
void join(Client *usr, std::string params, std::vector<Channel> &channels, Server *server);
void part(Client *usr, std::string params, std::vector<Channel> &channels);
int myStoi(const std::string &str);
