
#include <string>
#include <vector>
#include <netinet/in.h>
#include "Client.hpp"
#include <iostream>

class Channel
{
    typedef struct s_modes{
    bool i;
    bool t;
    bool k;
    bool o;
    bool l;
    } t_modes;

    private:
        std::string _id;
        std::string topic;
        size_t userlimit;
        std::string password;
        std::vector<Client *> users;
        std::vector<Client *> operators;
        std::vector<std::string> invitelist;
    private:
        t_modes modes;
    public:
        Channel(std::string name);
        std::vector<Client *> getusers();
        std::string         getid();
        std::string         gettopic();
        void                changetopic(std::string topic);
        void                changemode(std::string);
        void                changepw(std::string pw);
        void                changeul(size_t ul);
        std::string         getmode();
        size_t              getusercount();
        size_t              getuserlimit();
        std::string         getpw();
        bool                adduser(Client * user);
        bool                addoperator(Client * user);
        bool                istopicprotected();
        void                deluser(Client user);
        void                deloperator(Client user);
        bool                isoperator(Client user);
        bool                isinvited(Client user);
        bool                iscommand(std::string);
        bool                dispatchmessage(std::string);
};