#pragma once
#include <string>
#include <vector>

class Client;
class Channel;

bool handle_server_command(Client &client, const std::string &command, const std::string &params, std::vector<Client> &clients, std::vector<Channel> &channels, const std::string &PASSWORD);
