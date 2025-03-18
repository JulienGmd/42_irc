#include "serverCommands.hpp"
#include "Utils.hpp"
#include "_config.hpp"
#include "channelCommands.hpp"

#include <vector>

void nick_cmd(Client &client, const std::string &params, const std::vector<Client> &clients)
{
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    std::cout << std::endl
              << "params" << std::endl;
    std::cout << params << std::endl;

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

    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i].username == newNickname)
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

void user_cmd(Client &client, const std::string &params, const std::vector<Client> &clients)
{
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

    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i].username == newUsername)
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

void invite_cmd(Client &client, const std::string &params, std::vector<Client> &clients, std::vector<Channel> &channels)
{
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
    for (size_t i = 0; i < clients.size(); i++)
    {
        if (targetUser == clients[i].nickname)
        {
            target = &clients[i];
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

void prv_msg(Client &client, const std::string &params, const std::vector<Client> &clients)
{
    std::string hostname = IRCHOSTNAME;

    // compare socket to found client
    std::vector<Client> users = clients;
    Client *target = NULL;
    Client *sender = &client;
    std::vector<std::string> splitParams = splitString(params, ' ');

    for (size_t j = 0; j < users.size(); j++)
    {
        if (splitParams[0] == users[j].nickname)
        {
            target = &users[j];
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
    msgNotif << ":" << sender->nickname << " " << " PRIVMSG " << target->nickname << " :" << msg << "\r\n";
    send(target->socket, msgNotif.str().c_str(), msgNotif.str().length(), 0);
}
