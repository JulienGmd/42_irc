#pragma once

#include "Client.hpp"
#include "Server.hpp"

bool handle_channel_command(Client usr, std::string command, std::string params, std::vector<Channel> &channels);
void join(Client usr, std::string params, std::vector<Channel> &a);
