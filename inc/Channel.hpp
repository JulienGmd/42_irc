
#include <string>
#include <vector>
#include <netinet/in.h>
#include "Client.hpp"
#include <iostream>

typedef struct s_modes{
    bool i;
    bool t;
    bool k;
    bool o;
    bool l;
    std::vector<std::string> invitelist;
    std::string password;
    size_t userlimit;
} t_modes;

class Channel 
{
    private:
        std::string _id;
        std::vector<Client *> users;
        std::vector<Client *> operators;
        std::string topic;
    
    private:
        t_modes modes;
    public:
        Channel(std::string name);
        
        std::string         getid();
        std::string         gettopic();
        void                changetopic(std::string topic);
        void                changemode(std::string);
        std::string         getmode();
        bool                adduser(Client * user);
        void                deluser(Client user);
        bool                isoperator(Client user);
        bool                iscommand(std::string);
        bool                dispatchmessage(std::string);
};