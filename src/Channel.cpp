#include "Channel.hpp"
#include <sstream>
#include "_config.hpp"

Channel::Channel(std::string name) : _id(name), topic(""), userlimit(0), password(""){
    modes.i = 0;
    modes.t = 0;
    modes.k = 0;
    modes.o = 0;
    modes.l = 0;
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
    if (modes.l)
        amodes += "l";
    return (amodes);
}

std::vector<Client *> Channel::getusers()
{
    return(users);
}

void                Channel::changepw(std::string pw)
{
    password = pw;
}

void                Channel::changeul(size_t ul)
{
    userlimit = ul;
}

bool Channel::hasuser(Client& usr) {
    for (size_t i = 0; i < users.size(); i++) {
        if (users[i]->socket == usr.socket) {
            return true;
        }
    }
    return false;
}

void Channel::applymode(const std::string& modes, const std::vector<std::string>& params, Client* usr, std::vector<Channel>& channels) {
    (void)usr;      // Suppress unused parameter warning
    (void)channels; // Suppress unused parameter warning
    bool addMode = true; // '+' means add mode, '-' means remove mode
    size_t paramIndex = 0;

    for (size_t i = 0; i < modes.size(); i++) {
        char mode = modes[i];

        if (mode == '+') {
            addMode = true;
        } else if (mode == '-') {
            addMode = false;
        } else if (mode == 'i') {
            changemode(addMode ? "+i" : "-i");
        } else if (mode == 't') {
            changemode(addMode ? "+t" : "-t");
        } else if (mode == 'k') {
            if (addMode && paramIndex < params.size()) {
                changemode("+k");
                changepw(params[paramIndex++]);
            } else {
                changemode("-k");
                changepw("");
            }
        } else if (mode == 'l') {
            if (addMode && paramIndex < params.size()) {
                changemode("+l");
                changeul(myStoi(params[paramIndex++]));
            } else {
                changemode("-l");
                changeul(0); // Remove user limit
            }
        } else if (mode == 'o' && paramIndex < params.size()) {
            std::string targetNick = params[paramIndex++];
            Client* targetUser = NULL;

            for (size_t j = 0; j < users.size(); j++) {
                if (users[j]->nickname == targetNick) {
                    targetUser = users[j];
                    break;
                }
            }

            if (targetUser) {
                if (addMode) {
                    addoperator(targetUser);
                } else {
                    deloperator(*targetUser);
                }
            }
        }
    }
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
            else if (mode[i] == 'l')
                modes.l = 1;
            i++;
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
            else if (mode[i] == 'l')
                modes.l = 0;
            i++;
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

bool    Channel::istopicprotected()
{
    if (modes.t)
        return true;
    return false;
}

void    Channel::changetopic(std::string topic){
    this->topic = topic;
    std::string hostname = IRCHOSTNAME;
    std::string whobase = ":" + hostname + " 332 ";
    std::string whoreturn;
    std::vector<Client *> vec = users;
    for (size_t j = 0; j < vec.size(); j++)
    {
        whoreturn = whobase;
        whoreturn += vec[j]->nickname + " " + _id + " :" + topic + "\r\n";
        send(vec[j]->socket, whoreturn.c_str(), whoreturn.length(), 0);
    }
}

bool Channel::adduser(Client * user)
{
    for (size_t i = 0; i < users.size(); i++){
        if (users[i]->nickname == user->nickname)
            return (0);
    }
    users.push_back(user);
    return (1);
}

bool Channel::addoperator(Client * user)
{
    for (size_t i = 0; i < operators.size(); i++)
        if (operators[i]->nickname == user->nickname)
            return (0);
    operators.push_back(user);
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
            return;
        }
}

void Channel::deloperator(Client user)
{
    std::vector<Client *>::iterator it = operators.begin();

    for (size_t i = 0; it != operators.end(); it++, i++)
        if (operators[i]->nickname == user.nickname)
        {
            std::cerr << "Erasing \'" << user.nickname << "\' from " << this->_id  << " OPLIST"<< std::endl;
            operators.erase(it);
            break;
        }
}

bool Channel::isoperator(Client user)
{
    for (size_t i = 0; i < operators.size(); i++)
        if (operators[i]->nickname == user.nickname)
            return(1);
    return (0);
}

bool Channel::isinvited(Client user)
{
    for (size_t i = 0; i < invitelist.size(); i++)
        if (invitelist[i] == user.nickname)
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

size_t              Channel::getusercount()
{
    return(users.size());
}

size_t              Channel::getuserlimit()
{
    return (userlimit);
}
std::string         Channel::getpw()
{
    return(password);
}

std::string Channel::getnicklist(){
    std::ostringstream nicklist;

    for (size_t i = 0; i < users.size(); i++) {
        if (isoperator(*users[i])) {
            nicklist << "@";
        }
        nicklist << users[i]->nickname;

        // Add a space after each nickname, except the last one
        if (i < users.size() - 1) {
            nicklist << " ";
        }
    }

    return nicklist.str();
}