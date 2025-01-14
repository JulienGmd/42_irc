
#include "commands.hpp"
#include <algorithm>
#include <cstring>
#include <string>

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
    return false;
}

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
            return;
        }
    }
    if (i == a.size())
        std::cerr << "Nosuchchannel" << std::endl;
}
