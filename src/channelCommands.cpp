
#include "channelCommands.hpp"
#include <algorithm>
#include <cstring>
#include <string>

bool isallowed(Client usr, Channel chan, std::string pw)
{
    std::string modes = chan.getmode();
    for (size_t i = 0; modes[i]; i++)
    {
        if (modes[i] == 'i' && !chan.isinvited(usr))
            return (0);
        if (modes[i] == 'k' && pw != chan.getpw())
            return (0);
        if (modes[i] == 'l' &&  chan.getusercount() > chan.getuserlimit())
            return (0);
    }
    return (1);
}

void join(Client usr, std::string params, std::vector<Channel> &a)
{
    size_t i = 0;

    std::string pw;
    size_t j = 0;
    for (; params[j] != ' ' || params[j]; j++);
    pw = params.substr(j, params.length() - j);
    params = params.substr(0, j);
    std::string client = ":";
    client += usr.nickname + "!" + usr.username + "@" + usr.hostname + " ";
    for (; i < a.size(); i++)
    {
        if (a[i].getid() == params)
        {
            if (isallowed(usr, a[i], params))
            {
                std::string joinsuccess = client + " JOIN " + params + "\n";
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
        return true;
    }
    else if (command == "MODE")
    {
        return true;
    }
    return false;
}

