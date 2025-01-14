
#include "commands.hpp"
#include <algorithm>
#include <string>
#include <cstring>

void    join(int usrSocket, std::string params, std::vector<Channel> &a)
{
    size_t i = 0;

    std::string success = ":devduck!devduck@localhost JOIN general\n";
    std::replace(params.begin(), params.end(), '\n', '\0');
    for (; i < a.size(); i++)
    {
        if (strcmp(a[i].getid().c_str(), params.c_str()))
        {
            std::cerr << a[i].getid() << "|" << params << std::endl;
            send(usrSocket, success.c_str(), success.length(), 0);
            return;
        }
    }
    if (i == a.size())
        std::cerr << "Nosuchchannel" << std::endl;
}