#include "Channel.hpp"

Channel::Channel(std::string name) : _id(name), topic(""){
    modes.i = 0;
    modes.t = 0;
    modes.k = 0;
    modes.o = 0;
    modes.l = 0;
    modes.userlimit = 0;
}

std::string Channel::getmode()
{
    std::string amodes;

    if (modes.i)
        amodes += "i";
    if (modes.t)
        amodes += "t";
    if (modes.k)
        amodes += "k";
    if (modes.o)
        amodes += "o";
    if (modes.l)
        amodes += "l";
}

void        Channel::changemode(std::string mode)
{
    size_t i = 1;
    if (mode[0] == '+')
    {
        while (mode[i])
        {
            if (mode[i] == 'i')
                modes.i = 1;
            else if (mode[i] == 't')
                modes.t = 1;
            else if (mode[i] == 'k')
                modes.k = 1;
            else if (mode[i] == 'o')
                modes.o = 1;
            else if (mode[i] == 'l')
                modes.l = 1;
        }
    }
    else if (mode[0] == '-')
    {
        while (mode[i])
        {
            if (mode[i] == 'i')
                modes.i = 0;
            else if (mode[i] == 't')
                modes.t = 0;
            else if (mode[i] == 'k')
                modes.k = 0;
            else if (mode[i] == 'o')
                modes.o = 0;
            else if (mode[i] == 'l')
                modes.l = 0;
        }
    }
}

std::string    Channel::getid()
{
    return (this->_id);
}

std::string    Channel::gettopic()
{
    return (topic);
}

void    Channel::changetopic(std::string topic){
    this->topic = topic;
}

bool Channel::adduser(Client * user)
{
    for (size_t i = 0; i < users.size(); i++)
        if (users[i]->nickname == user->nickname)
            return (0);
    users.push_back(user);
    return (1);
}

void Channel::deluser(Client user)
{
    std::vector<Client *>::iterator it = users.begin();

    for (size_t i = 0; it != users.end(); it++, i++)
        if (users[i]->nickname == user.nickname)
        {
            std::cerr << "Erasing \'" << user.nickname << "\' from " << this->_id << std::endl;
            users.erase(it);
        }
}

bool Channel::isoperator(Client user)
{
    for (size_t i = 0; i < operators.size(); i++)
        if (operators[i]->nickname == user.nickname)
            return(1);
    return (0);
}

bool Channel::iscommand(std::string command)
{
    return (command == "NICK" || command == "JOIN"
        || command == "PART" || command == "PRIVMSG"
            || command == "QUIT" || command == "LIST");
}

bool Channel::dispatchmessage(std::string msg)
{
    if (!iscommand(msg))
    {
        for (size_t i = 0; i < users.size(); i++)
	        if (users[i]->socket != users[i]->socket)
	            send(users[i]->socket, msg.c_str(), msg.length(), 0);
    }
    else
        /*TODO : Execute commands*/return (0);
    return (1);
}