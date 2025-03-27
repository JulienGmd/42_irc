#include "serverCommands.hpp"
#include "Utils.hpp"
#include "_config.hpp"
#include "channelCommands.hpp"

#include <map>
#include <string>
#include <vector>

bool pass_cmd(Client &client, const std::string &params, const std::string &PASSWORD);
void nick_cmd(Client &client, const std::string &params, std::map<int, Client> &clients);
void user_cmd(Client &client, const std::string &params, std::map<int, Client> &clients);
void invite_cmd(Client &client, const std::string &params, std::map<int, Client> &clients, std::vector<Channel> &channels);
void prv_msg(Client &client, const std::string &params, std::map<int, Client> &clients);

/** @return false if the client should be disconnected, true otherwise. */
bool handle_server_command(Client &client, const std::string &command, const std::string &params, std::map<int, Client> &clients, std::vector<Channel> &channels, const std::string &PASSWORD)
{
    if (command == "PASS")
        return pass_cmd(client, params, PASSWORD);
    else if (command == "NICK")
        nick_cmd(client, params, clients);
    else if (command == "USER")
        user_cmd(client, params, clients);
    else if (command == "INVITE")
        invite_cmd(client, params, clients, channels);
    else if (command == "PRIVMSG")
        prv_msg(client, params, clients);
    else if (command == "QUIT")
        return false;
    return true;
}

bool pass_cmd(Client &client, const std::string &params, const std::string &PASSWORD)
{
    if (params != PASSWORD)
    {
        send(client.socket, "Invalid password\n", 17, 0);
        return false;
    }
    client.has_set_server_password = true;
    return true;
}

void nick_cmd(Client &client, const std::string &params, std::map<int, Client> &clients)
{
    if (!client.has_set_server_password)
        return;

    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.size() == 0)
    {
        std::ostringstream error;
        error << ":" << hostname << " " << ERR_NEEDMOREPARAMS << " " << client.nickname << " NICK :Not enough parameters\r\n";
        send(client.socket, error.str().c_str(), error.str().length(), 0);
        return;
    }

    std::string newNickname;

    for (size_t i = 0; i < split.size(); i++)
    {
        newNickname += split[i];
    }

    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        Client &client = it->second;
        if (client.username == newNickname)
        {
            std::ostringstream error;
            error << ":" << hostname << " " << ERR_ALREADYREGISTERED << " " << client.nickname << " NICK :NICK already exist\r\n";
            send(client.socket, error.str().c_str(), error.str().length(), 0);
            return;
        }
    }

    client.nickname = newNickname;

    // std::ostringstream msg_confirm;
    // msg_confirm << ":" << IRCHOSTNAME << " 001 " << newNickname << " :NICK is now " << newNickname << "\r\n";
    // send(client.socket, msg_confirm.str().c_str(), msg_confirm.str().length(), 0);
}

void user_cmd(Client &client, const std::string &params, std::map<int, Client> &clients)
{
    if (!client.has_set_server_password || client.nickname.empty())
        return;

    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.size() == 0)
    {
        std::ostringstream error;
        error << ":" << hostname << " " << ERR_NEEDMOREPARAMS << " " << client.nickname << " USER :Not enough parameters\r\n";
        send(client.socket, error.str().c_str(), error.str().length(), 0);
        return;
    }

    std::string newUsername;
    for (size_t i = 0; i < split.size(); i++)
    {
        newUsername += split[i];
    }

    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        Client &client = it->second;
        if (client.username == newUsername)
        {
            std::ostringstream error;
            error << ":" << hostname << " " << ERR_ALREADYREGISTERED << " " << client.nickname << " USER :username already exist\r\n";
            send(client.socket, error.str().c_str(), error.str().length(), 0);
            return;
        }
    }
    client.username = newUsername;

    // std::ostringstream msg_confirm;
    // msg_confirm << ":" << IRCHOSTNAME << " 001 " << client.nickname << " :User is now " << newUsername << "\r\n";
    // send(client.socket, msg_confirm.str().c_str(), msg_confirm.str().length(), 0);
}

void invite_cmd(Client &client, const std::string &params, std::map<int, Client> &clients, std::vector<Channel> &channels)
{
    if (!client.has_set_server_password)
        return;

    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    // check if bad arguments
    if (split.size() < 2)
    {
        std::ostringstream error;
        error << ":" << hostname << " " << ERR_NEEDMOREPARAMS << " " << client.nickname << " INVITE :Not enough parameters\r\n";
        send(client.socket, error.str().c_str(), error.str().length(), 0);
        return;
    }

    std::string targetUser = split[0];
    std::string targetChannel = split[1];
    Channel *channel = NULL;

    for (size_t i = 0; i < channels.size(); i++)
    {
        if (targetChannel == channels[i].getid())
        {
            channel = &channels[i];
            break;
        }
    }

    if (!channel)
    {
        std::ostringstream error;
        error << ":" << hostname << " " << ERR_NOSUCHCHANNEL << " " << client.nickname << " " << targetChannel << " :No such channel\r\n";
        send(client.socket, error.str().c_str(), error.str().length(), 0);
        return;
    }

    // check if the user is on the channel
    if (!channel->hasuser(client))
    {
        std::ostringstream error;
        error << ":" << hostname << " " << ERR_NOTONCHANNEL << " " << client.nickname << " " << targetChannel << " :You're not on that channel\r\n";
        send(client.socket, error.str().c_str(), error.str().length(), 0);
        return;
    }

    // check if the user is operator
    if (!channel->isoperator(client))
    {
        std::ostringstream error;
        error << ":" << hostname << " " << ERR_CHANOPRIVSNEEDED << " " << client.nickname << " " << targetChannel << " :You're not channel operator\r\n";
        send(client.socket, error.str().c_str(), error.str().length(), 0);
        return;
    }

    // look for the target user is connected
    // need list of clients
    Client *target = NULL;
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        Client &client = it->second;
        if (targetUser == client.nickname)
        {
            target = &client;
            break;
        }
    }

    if (!target)
    {
        std::ostringstream error;
        error << ":" << hostname << " " << ERR_NOSUCHNICK << " " << client.nickname << " " << targetUser << " :No such nick/channel\r\n";
        send(client.socket, error.str().c_str(), error.str().length(), 0);
        return;
    }

    // send invite msg channel
    std::ostringstream msg_invite;
    msg_invite << ":" << client.nickname << " INVITE " << targetUser << " " << targetChannel << "\r\n";
    send(target->socket, msg_invite.str().c_str(), msg_invite.str().length(), 0);

    channel->addinvited(*target);

    // confirm invite msg sent
    std::ostringstream msg_confirm;
    msg_confirm << ":" << hostname << " " << RPL_INVITING << "" << client.nickname << " " << targetUser << " " << targetChannel.substr(1) << " :Invite sent\r\n";
    send(client.socket, msg_confirm.str().c_str(), msg_confirm.str().length(), 0);
}

void prv_msg(Client &client, const std::string &params, std::map<int, Client> &clients)
{
    if (!client.has_set_server_password)
        return;

    std::string hostname = IRCHOSTNAME;

    // compare socket to found client
    Client *target = NULL;
    std::vector<std::string> splitParams = splitString(params, ' ');

    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        Client &client = it->second;
        if (splitParams[0] == client.nickname)
        {
            target = &client;
            break;
        }
    }
    if (!target)
    {
        std::ostringstream error;
        error << ":" << hostname << " " << ERR_NOSUCHNICK << " " << client.nickname << " " << splitParams[0] << " :No such nick\r\n";
        send(client.socket, error.str().c_str(), error.str().length(), 0);
        return;
    }
    // <Client> PRIVMSG <Target> :<Message>
    std::ostringstream msgNotif;
    std::string msg = "";
    for (size_t i = 1; i < splitParams.size(); i++)
    {
        msg += splitParams[i];
        if (i != splitParams.size() - 1)
            msg += " ";
    }
    if (msg[0] == ':')
        msg = msg.substr(1, msg.size() - 1);
    msgNotif << ":" << client.nickname << " " << " PRIVMSG " << target->nickname << " :" << msg << "\r\n";
    send(target->socket, msgNotif.str().c_str(), msgNotif.str().length(), 0);
}
