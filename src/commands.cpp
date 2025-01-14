
#include "commands.hpp"
#include <algorithm>
#include <cstring>
#include <string>

void join(Client usr, std::string params, std::vector<Channel> &a)
{
    size_t i = 0;

    std::string success = ":";
    success += usr.nickname + "!" + usr.username + "@" + usr.hostname + " JOIN " + params + "\n";
    for (; i < a.size(); i++)
    {
        if (a[i].getid() == params)
        {
            send(usr.socket, success.c_str(), success.length(), 0);
            send(usr.socket);
            return;
        }
    }
    if (i == a.size())
        std::cerr << "Nosuchchannel" << std::endl;
}

bool handle_channel_command(Client usr, std::string command, std::string params, std::vector<Channel> &channels)
{
    if (command == "JOIN")
    {
        join(usr, params, channels);
        return true;
    }
    else if (command == "PART")
    {
        // TODO disconnect client from channel
        return true;
    }
    else if (command == "LIST")
    {
        // TODO list channels
        return true;
    }
    else if (command == "WHO")
    {

    }
    else if (command == "MODE")
    {

    }
    return false;
}

