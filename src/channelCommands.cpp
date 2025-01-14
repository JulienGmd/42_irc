
#include "channelCommands.hpp"
#include <algorithm>
#include <cstring>
#include <string>

bool isallowed(Client usr, Channel chan)
{
    return (1);
}

void join(Client usr, std::string params, std::vector<Channel> &a)
{
    size_t i = 0;

    std::string client = ":";
    client += usr.nickname + "!" + usr.username + "@" + usr.hostname + " ";
    for (; i < a.size(); i++)
    {
        if (a[i].getid() == params)
        {
            if (isallowed(usr, a[i]))
            {
                std::string joinsuccess = client + " JOIN " + params + "\n";
                std::string topicsuccess= client + ;
                joinsuccess += ;
                send(usr.socket, joinsuccess.c_str(), joinsuccess.length(), 0);
                // send(usr.socket, );
            }
            return;
        }
    }
    if (i == a.size())
    {
        Channel newchan(params);
    }
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

