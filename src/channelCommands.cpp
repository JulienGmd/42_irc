#include "channelCommands.hpp"
#include "_config.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

// Helper Functions
int myStoi(const std::string &str)
{
    int result = 0;
    int sign = 1;
    size_t i = 0;

    if (str.empty())
    {
        throw std::invalid_argument("Input string is empty");
    }

    if (str[i] == '+' || str[i] == '-')
    {
        sign = (str[i] == '-') ? -1 : 1;
        i++;
    }

    for (; i < str.size(); i++)
    {
        if (str[i] < '0' || str[i] > '9')
        {
            throw std::invalid_argument("Invalid character in input string");
        }
        int digit = str[i] - '0';

        if (result > (2147483647 - digit) / 10)
        {
            throw std::out_of_range("Integer overflow");
        }

        result = result * 10 + digit;
    }

    return result * sign;
}

std::vector<std::string> splitString(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;

    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
    {
        if (*it == delimiter)
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
        }
        else
        {
            token += *it;
        }
    }
    if (!token.empty())
    {
        tokens.push_back(token);
    }
    return tokens;
}

bool isallowed(Client usr, Channel chan, std::string pw)
{
    std::string modes = chan.getmode();
    for (size_t i = 0; i < modes.size(); i++)
    {
        if (modes[i] == 'i' && !chan.isinvited(usr))
        {
            return false;
        }
        else if (modes[i] == 'k' && pw != chan.getpw())
        {
            return false;
        }
        else if (modes[i] == 'l' && chan.getusercount() >= chan.getuserlimit())
        {
            return false;
        }
    }
    return true;
}

void join_channel(Client *usr, Channel &channel)
{
    std::string hostname = IRCHOSTNAME;
    std::string channelName = channel.getid();

    channel.adduser(usr);
    if (channel.getusers().size() == 1)
    {
        channel.addoperator(usr);
    }

    std::ostringstream joinMsg;
    joinMsg << ":" << usr->nickname << "!" << usr->username << "@" << usr->hostname << " JOIN " << channelName << "\r\n";
    std::vector<Client *> users = channel.getusers();
    std::vector<Client *>::iterator it = users.begin();
    for (size_t j = 0; j < users.size(); j++)
    {
        send((*it)->socket, joinMsg.str().c_str(), joinMsg.str().length(), MSG_NOSIGNAL);
        it++;
    }

    std::ostringstream topicMsg;
    topicMsg << ":" << hostname << " 332 " << usr->nickname << " " << channelName << " :" << channel.gettopic() << "\r\n";
    send(usr->socket, topicMsg.str().c_str(), topicMsg.str().length(), MSG_NOSIGNAL);

    for (size_t j = 0; j < users.size(); j++)
    {
        std::ostringstream namesMsg;
        namesMsg << ":" << hostname << " 353 " << users[j]->nickname << " = " << channelName << " :" << channel.getnicklist() << "\r\n";
        send(users[j]->socket, namesMsg.str().c_str(), namesMsg.str().length(), MSG_NOSIGNAL);

        std::ostringstream endNamesMsg;
        endNamesMsg << ":" << hostname << " 366 " << users[j]->nickname << " " << channelName << " :End of /NAMES list\r\n";
        send(users[j]->socket, endNamesMsg.str().c_str(), endNamesMsg.str().length(), MSG_NOSIGNAL);
    }
}

// JOIN Command
void join(Client *usr, std::string params, std::vector<Channel> &channels, Server *server)
{
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.empty())
    {
        std::ostringstream error;
        error << ":" << hostname << " 461 " << usr->nickname << " JOIN :Not enough parameters\r\n";
        send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
        return;
    }

    std::string channelName = split[0];
    std::string password = (split.size() > 1) ? split[1] : "";

    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getid() == channelName)
        {
            if (!isallowed(*usr, channels[i], password))
            {
                std::ostringstream error;
                error << ":" << hostname << " 474 " << usr->nickname << " " << channelName << " :Cannot join channel\r\n";
                send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
                return;
            }
            join_channel(usr, channels[i]);
            return;
        }
    }
    join_channel(usr, *server->add_channel(channelName));
}

// PART Command
void part(Client *usr, std::string params, std::vector<Channel> &channels)
{
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.empty())
    {
        std::ostringstream error;
        error << ":" << hostname << " 461 " << usr->nickname << " PART :Not enough parameters\r\n";
        send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
        return;
    }

    std::string channelName = split[0];
    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getid() == channelName)
        {
            std::vector<Client *> users = channels[i].getusers();
            bool userFound = false;

            for (size_t j = 0; j < users.size(); j++)
            {
                if (users[j]->socket == usr->socket)
                {
                    userFound = true;
                    break;
                }
            }

            if (!userFound)
            {
                std::ostringstream fail;
                fail << ":" << hostname << " 442 " << usr->nickname << " " << channelName << " :You're not on that channel\r\n";
                send(usr->socket, fail.str().c_str(), fail.str().length(), MSG_NOSIGNAL);
                return;
            }

            std::ostringstream partMsg;
            partMsg << ":" << usr->nickname << "!" << usr->username << "@" << usr->hostname << " PART " << channelName << "\r\n";
            std::vector<Client *> userslist = channels[i].getusers();
            for (size_t j = 0; j < userslist.size(); j++)
            {
                send(userslist[j]->socket, partMsg.str().c_str(), partMsg.str().length(), MSG_NOSIGNAL);
            }
            channels[i].deluser(*usr);

            return;
        }
    }

    std::ostringstream fail;
    fail << ":" << hostname << " 403 " << usr->nickname << " " << channelName << " :No such channel\r\n";
    send(usr->socket, fail.str().c_str(), fail.str().length(), MSG_NOSIGNAL);
}

// WHO Command
bool who(Client usr, std::string params, std::vector<Channel> &channels)
{
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.size() > 1)
    {
        std::ostringstream error;
        error << ":" << hostname << " 461 " << usr.nickname << " WHO :Not enough parameters\r\n";
        send(usr.socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
        return false;
    }

    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getid() == split[0])
        {
            std::vector<Client *> users = channels[i].getusers();
            std::ostringstream nicklist;

            for (size_t j = 0; j < users.size(); j++)
            {
                if (channels[i].isoperator(*users[j]))
                {
                    nicklist << "@";
                }
                nicklist << users[j]->nickname;
                if (j < users.size() - 1)
                {
                    nicklist << " ";
                }
            }

            std::ostringstream whobase;
            whobase << ":" << hostname << " 353 " << usr.nickname << " = " << split[0] << " :" << nicklist.str() << "\r\n";
            send(usr.socket, whobase.str().c_str(), whobase.str().length(), MSG_NOSIGNAL);

            std::ostringstream whoend;
            whoend << ":" << hostname << " 366 " << usr.nickname << " " << split[0] << " :End of /NAMES list\r\n";
            send(usr.socket, whoend.str().c_str(), whoend.str().length(), MSG_NOSIGNAL);

            return true;
        }
    }

    std::ostringstream joinfail;
    joinfail << ":" << hostname << " 403 " << usr.nickname << " " << params << " :No such channel\r\n";
    send(usr.socket, joinfail.str().c_str(), joinfail.str().length(), MSG_NOSIGNAL);
    return false;
}

// KICK Command
void kick(Client *usr, std::string params, std::vector<Channel> &channels)
{
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.size() < 2)
    {
        std::ostringstream error;
        error << ":" << hostname << " 461 " << usr->nickname << " KICK :Not enough parameters\r\n";
        send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
        return;
    }

    std::string channelName = split[0];
    std::string targetName = split[1];
    std::string reason = (split.size() > 2) ? params.substr(params.find(targetName) + targetName.length() + 1) : "No reason provided";

    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getid() == channelName)
        {
            if (!channels[i].isoperator(*usr))
            {
                std::ostringstream fail;
                fail << ":" << hostname << " 482 " << usr->nickname << " " << channelName << " :You're not channel operator\r\n";
                send(usr->socket, fail.str().c_str(), fail.str().length(), MSG_NOSIGNAL);
                return;
            }

            std::vector<Client *> users = channels[i].getusers();
            bool userFound = false;
            for (size_t j = 0; j < users.size(); j++)
            {
                if (users[j]->nickname == targetName)
                {
                    userFound = true;

                    std::ostringstream kickMsg;
                    kickMsg << ":" << usr->nickname << "!" << usr->username << "@" << usr->hostname << " KICK " << channelName << " " << targetName << " :" << reason << "\r\n";
                    std::vector<Client *> userslist = channels[i].getusers();
                    for (size_t j = 0; j < userslist.size(); j++)
                    {
                        send(userslist[j]->socket, kickMsg.str().c_str(), kickMsg.str().length(), MSG_NOSIGNAL);
                    }

                    channels[i].deluser(*users[j]);
                    break;
                }
            }

            if (!userFound)
            {
                std::ostringstream error;
                error << ":" << hostname << " 441 " << targetName << " " << channelName << " :They aren't on that channel\r\n";
                send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
            }
            return;
        }
    }

    std::ostringstream error;
    error << ":" << hostname << " 403 " << usr->nickname << " " << channelName << " :No such channel\r\n";
    send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
}

// TOPIC Command
void topic(Client *usr, std::string params, std::vector<Channel> &channels)
{
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.empty())
    {
        std::ostringstream error;
        error << ":" << hostname << " 461 " << usr->nickname << " TOPIC :Not enough parameters\r\n";
        send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
        return;
    }

    std::string channelName = split[0];
    std::string newTopic = (split.size() > 1) ? params.substr(params.find(' ') + 1) : "";

    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getid() == channelName)
        {
            if (!channels[i].isoperator(*usr) && channels[i].istopicprotected() && !newTopic.empty())
            {
                std::ostringstream error;
                error << ":" << hostname << " 482 " << usr->nickname << " " << channelName << " :You're not channel operator\r\n";
                send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
                return;
            }

            if (newTopic.empty())
            {
                // Send current topic
                std::ostringstream topicMsg;
                topicMsg << ":" << hostname << " 332 " << usr->nickname << " " << channelName << " :" << channels[i].gettopic() << "\r\n";
                send(usr->socket, topicMsg.str().c_str(), topicMsg.str().length(), MSG_NOSIGNAL);
            }
            else
            {
                // Change topic
                channels[i].changetopic(newTopic);

                std::ostringstream topicChange;
                topicChange << ":" << usr->nickname << "!" << usr->username << "@" << usr->hostname << " TOPIC " << channelName << " :" << newTopic << "\r\n";

                std::vector<Client *> users = channels[i].getusers();
                for (size_t j = 0; j < users.size(); j++)
                {
                    send(users[j]->socket, topicChange.str().c_str(), topicChange.str().length(), MSG_NOSIGNAL);
                }
            }
            return;
        }
    }

    std::ostringstream error;
    error << ":" << hostname << " 403 " << usr->nickname << " " << channelName << " :No such channel\r\n";
    send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
}

// MODE Command
void mode(Client *usr, std::string params, std::vector<Channel> &channels)
{
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.empty())
    {
        // Missing parameters
        std::ostringstream error;
        error << ":" << hostname << " 461 " << usr->nickname << " MODE :Not enough parameters\r\n";
        send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
        return;
    }

    std::string channelName = split[0];
    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getid() == channelName)
        {
            // If no additional parameters, just return the current mode
            if (split.size() == 1)
            {
                std::ostringstream modeMsg;
                modeMsg << ":" << hostname << " 324 " << usr->nickname << " " << channelName << " :" << channels[i].getmode() << "\r\n";
                send(usr->socket, modeMsg.str().c_str(), modeMsg.str().length(), MSG_NOSIGNAL);

                std::ostringstream endModeMsg;
                endModeMsg << ":" << hostname << " 329 " << usr->nickname << " " << channelName << " :" << time(NULL) << "\r\n";
                send(usr->socket, endModeMsg.str().c_str(), endModeMsg.str().length(), MSG_NOSIGNAL);
                return;
            }

            // Handle mode changes if there are more parameters
            std::string modes = split[1];
            std::vector<std::string> modeParams(split.begin() + 2, split.end());
            channels[i].applymode(modes, modeParams, usr, channels);

            // Notify all users in the channel about the mode change
            std::ostringstream notifyMsg;
            notifyMsg << ":" << usr->nickname << "!" << usr->username << "@" << usr->hostname << " MODE " << channelName << " " << modes;
            for (size_t j = 0; j < modeParams.size(); j++)
            {
                notifyMsg << " " << modeParams[j];
            }
            notifyMsg << "\r\n";

            std::vector<Client *> users = channels[i].getusers();
            for (size_t j = 0; j < users.size(); j++)
            {
                send(users[j]->socket, notifyMsg.str().c_str(), notifyMsg.str().length(), MSG_NOSIGNAL);
            }

            return;
        }
    }

    // Channel not found
    std::ostringstream error;
    error << ":" << hostname << " 403 " << usr->nickname << " " << channelName << " :No such channel\r\n";
    send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
}

// PRIVMSG Command
bool privmsg(Client *usr, std::string params, std::vector<Channel> &channels)
{
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.size() < 2)
    {
        std::ostringstream error;
        error << ":" << hostname << " 461 " << usr->nickname << " PRIVMSG :Not enough parameters\r\n";
        send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
        return true;
    }
    std::string target = split[0];

    std::string message = params.substr(params.find(' ') + 2);

    if (target[0] == '#')
    {
        // Message to a channel
        for (size_t i = 0; i < channels.size(); i++)
        {
            if (channels[i].getid() == target)
            {
                if (!channels[i].hasuser(*usr))
                {
                    std::ostringstream error;
                    error << ":" << hostname << " 404 " << usr->nickname << " " << target << " :Cannot send to channel\r\n";
                    send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
                    return true;
                }

                std::ostringstream msgNotif;
                msgNotif << ":" << usr->nickname << "!" << usr->username << "@" << usr->hostname << " PRIVMSG " << target << " :" << message << "\r\n";

                std::vector<Client *> users = channels[i].getusers();
                for (size_t j = 0; j < users.size(); j++)
                {
                    if (users[j]->socket != usr->socket)
                    {
                        send(users[j]->socket, msgNotif.str().c_str(), msgNotif.str().length(), MSG_NOSIGNAL);
                    }
                }
                return true;
            }
        }

        std::ostringstream error;
        error << ":" << hostname << " 403 " << usr->nickname << " " << target << " :No such channel\r\n";
        send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
        return true;
    }
    else
    {
        return false;
        // Message to another user (not implemented in this scope)
        // std::ostringstream error;
        // error << ":" << hostname << " 401 " << usr->nickname << " " << target << " :No such nick/channel\r\n";
        // send(usr->socket, error.str().c_str(), error.str().length(), MSG_NOSIGNAL);
    }
}

/** @return True if the command was handled, false otherwise. */
bool handle_channel_command(Client *usr, std::string command, std::string params, std::vector<Channel> &channels, Server *server)
{
    if (!usr->has_set_server_password || usr->nickname.empty() || usr->username.empty())
        return false;

    if (command == "JOIN")
        join(usr, params, channels, server);
    else if (command == "PART")
        part(usr, params, channels);
    else if (command == "TOPIC")
        topic(usr, params, channels);
    else if (command == "KICK")
        kick(usr, params, channels);
    else if (command == "MODE")
        mode(usr, params, channels);
    else if (command == "PRIVMSG")
        return privmsg(usr, params, channels);
    else
        return false;
    return true;
}
