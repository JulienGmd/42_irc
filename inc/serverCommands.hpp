#pragma once
#include <string>
#include <vector>

class Client;
class Channel;

void nick_cmd(Client &client, const std::string &params, const std::vector<Client> &clients);
void user_cmd(Client &client, const std::string &params, const std::vector<Client> &clients);
void invite_cmd(Client &client, const std::string &params, std::vector<Client> &clients, std::vector<Channel> &channels);
void prv_msg(Client &client, const std::string &params, const std::vector<Client> &clients);
