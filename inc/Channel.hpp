#pragma once

#include "Client.hpp"
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <vector>

class Server;

int myStoi(const std::string &str);

class Channel
{
    typedef struct s_modes
    {
        bool i;
        bool t;
        bool k;
        bool o;
        bool l;
    } t_modes;

private:
    Server &server;
    std::string _id;
    std::string topic;
    size_t userlimit;
    std::string password;
    std::vector<int> user_keys;
    std::vector<int> operator_keys;
    std::vector<std::string> invitelist;

private:
    t_modes modes;

public:
    Channel(Server &server, std::string name);
    std::vector<Client *> getusers();
    std::vector<Client *> getoperators();
    std::string getid();
    std::string gettopic();
    void changetopic(std::string topic);
    void changemode(std::string);
    void applymode(const std::string &modes, const std::vector<std::string> &params, Client *usr, std::vector<Channel> &channels);
    void changepw(std::string pw);
    void changeul(size_t ul);
    std::string getmode();
    size_t getusercount();
    size_t getuserlimit();
    std::string getpw();
    bool adduser(Client *user);
    bool addoperator(Client *user);
    bool istopicprotected();
    std::string getnicklist();
    bool hasuser(Client &usr);
    void deluser(Client user);
    void deloperator(Client user);
    bool isoperator(Client user);
    bool isinvited(Client user);
    bool iscommand(std::string);
    bool dispatchmessage(std::string);
    void addinvited(Client user);
};
