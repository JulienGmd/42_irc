
#include <string>
#include <vector>
#include <netinet/in.h>
#include "Client.h"
#include <iostream>

class Channel 
{
    private:
        std::string _id;
        std::vector<Client *> users;
        std::vector<Client *> operators;
    public:
        bool adduser(Client * user);
        void deluser(Client user);
        bool isoperator(Client user);
        bool iscommand(std::string);
        bool dispatchmessage(std::string);
};