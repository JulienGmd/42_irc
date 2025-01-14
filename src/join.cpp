
#include "commands.hpp"
#include <algorithm>
#include <string>
#include <cstring>

void    join(Client usr, std::string params, std::vector<Channel> &a)
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