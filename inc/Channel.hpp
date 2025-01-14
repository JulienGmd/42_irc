
#include <string>
#include <vector>
#include <netinet/in.h>
#include "Client.hpp"
#include <iostream>

class Channel 
{
    private:
        std::string _id;
        std::vector<Client *> users;
        std::vector<Client *> operators;
        std::string topic;
    public:
        Channel(std::string name);
        std::string getid();
        std::string gettopic();
        void        changetopic(std::string topic);
        bool adduser(Client * user);
        void deluser(Client user);
        bool isoperator(Client user);
        bool iscommand(std::string);
        bool dispatchmessage(std::string);
};