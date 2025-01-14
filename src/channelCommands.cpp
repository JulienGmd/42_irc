
#include "channelCommands.hpp"
#include "_config.hpp"
#include <algorithm>
#include <cstring>
#include <string>

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;

    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (*it == delimiter) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += *it;
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}


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

void mode(Client *usr, std::string params, std::vector<Channel> &a)
{
    size_t i = 0;
    std::vector<std::string> split = splitString(params, ' ');
    std::string hostname = IRCHOSTNAME;
    for (; i < a.size(); i++)
    {
        if (a[i].getid() == split[0])
        {
            std::string modes = a[i].getmode();
            if (split.size() > 1)
            {
                // TODO : Modes managing
                (void)split;
            }
            else
            {
                std::string newmodes = "";
                if (modes[0])
                    newmodes += "+" + modes;
                std::string modereturn = ":" + hostname + " 324 " + usr->nickname + " " + split[0] + " :" + newmodes + "\r\n";
                std::cout << modereturn << std::endl;
                send(usr->socket, modereturn.c_str(), modereturn.length(), 0);
            }
            return;
        }
    }
    if (i == a.size())
    {
        std::string joinfail = ":" + hostname + " 403 " + usr->nickname + " " + params + " :No such channel" + "\r\n";
        send(usr->socket, joinfail.c_str(), joinfail.length(), 0);
    }
}

bool who(Client usr, std::string params, std::vector<Channel> &a)
{
    size_t i = 0;
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');
    if (split.size() > 1)
        return (0);
    for (; i < a.size(); i++)
    {
        if (a[i].getid() == split[0])
        {
                std::string whobase = ":" + hostname + " 353 ";
                std::string nicklist = "";
                std::vector<Client *> vec= a[i].getusers();
                for (size_t j = 0; j < a[i].getusercount(); j++)
                {
                    if (a[i].isoperator(usr))
                        nicklist += "@";
                    nicklist += vec[j]->nickname;
                    if (j != a[i].getusercount() - 1)
                        nicklist += ' ';
                }
                std::string whoreturn;
                std::string whoend;
                for (size_t j = 0; j < vec.size(); j++)
                {
                    whoreturn = whobase;
                    whoreturn += vec[j]->nickname + " = " + split[0] + " :"+ nicklist + "\r\n";
                    std::cout << whoreturn << std::endl;
                    send(vec[j]->socket, whoreturn.c_str(), whoreturn.length(), 0);
                    whoend = ":" + hostname + " 366 " + vec[j]->nickname + " " + split[0] + " :End of /NAMES list";
                    std::cerr << whoend << " to " << vec[j]->nickname << " on socket " << vec[j]->socket << std::endl;
                    send(vec[j]->socket, whoend.c_str(), whoend.length(), 0);
                }
            return 1;
        }
    }
    if (i == a.size())
    {
        std::string joinfail = ":" + hostname + " 403 " + usr.nickname + " " + params + " :No such channel" + "\r\n";
        send(usr.socket, joinfail.c_str(), joinfail.length(), 0);
    }
    return 1;
}

void join(Client *usr, std::string params, std::vector<Channel> &a)
{
    size_t i = 0;

    std::vector<std::string> split = splitString(params, ' ');
    std::string hostname = IRCHOSTNAME;
    params = split[0];
    std::string pw = "";
    if (split.size() > 1)
        pw = split[1];
    std::string client = ":";
    client += usr->nickname + "!" + usr->username + "@" + usr->hostname + " ";
    for (; i < a.size(); i++)
    {
        if (a[i].getid() == params)
        {
            if (isallowed(*usr, a[i], pw))
            {
                std::string topic = ":" + hostname + " 332 " + usr->nickname + " " + params+ " :" + a[i].gettopic() + "\r\n";
                std::string joinsuccess = client + " JOIN " + params + "\r\n";
                send(usr->socket, joinsuccess.c_str(), joinsuccess.length(), 0);
                send(usr->socket, topic.c_str(), topic.length(), 0);
                mode(usr, params, a);
                a[i].adduser(usr);
                who(*usr, params, a);
            }
            return;
        }
    }
    if (i == a.size())
    {
        std::string joinfail = ":" + hostname + " 403 " + usr->nickname + " " + params + " :No such channel" + "\r\n";
        send(usr->socket, joinfail.c_str(), joinfail.length(), 0);
    }
}

void msg(Client *usr, std::string params, std::vector<Channel> &a)
{
    size_t i = 0;
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');
    std::string allmessage;
    for (size_t j = 1; j < split.size(); j++)
    {
        if (j != 1)
            allmessage += " ";
        allmessage += split[j];
    }
    std::string msgnotif = ":" + usr->nickname + "!" + usr->username + "@" + usr->hostname + " PRIVMSG " + split[0] + " " + allmessage + "\r\n";
    ssize_t bytesread;
    for (; i < a.size(); i++)
    {
        if (a[i].getid() == split[0])
        {
                std::vector<Client *> vec= a[i].getusers();
                for (size_t j = 0; j < a[i].getusercount(); j++)
                {
                    if (vec[j]->socket != usr->socket)
                    {
                        int error = 0;
                        socklen_t len = sizeof(error);
                        int retval = getsockopt(vec[j]->socket, SOL_SOCKET, SO_ERROR, &error, &len);
                        if (retval == 0 && error == 0) {
                            std::cerr << "Socket for " << vec[j]->nickname << " is valid." << std::endl;
                        } else {
                            std::cerr << "Socket error for " << vec[j]->nickname << ": " << strerror(error) << std::endl;
                        }
                        bytesread = send(vec[j]->socket, msgnotif.c_str(), msgnotif.length(), 0);
                        if (bytesread == -1) {
                            std::cerr << "Error sending PRIVMSG";
                        } else {
                            std::cerr << "Sent " << bytesread << " bytes to " << vec[j]->nickname << " (Socket: " << vec[j]->socket << ")" << std::endl;
                        }
                    }
                }
                return;
        }
    }
    if (i == a.size())
    {
        std::string joinfail = ":" + hostname + " 403 " + usr->nickname + " " + params + " :No such channel" + "\r\n";
        send(usr->socket, joinfail.c_str(), joinfail.length(), 0);
    }
}

bool handle_channel_command(Client *usr, std::string command, std::string params, std::vector<Channel> &channels)
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
        mode(usr, params, channels);
        return true;
    }
    else if (command == "PRIVMSG")
    {
        msg(usr, params, channels);
    }
    return false;
}

