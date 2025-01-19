#include "channelCommands.hpp"
#include "_config.hpp"
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>

// Helper Functions
int myStoi(const std::string& str) {
    int result = 0;
    int sign = 1;
    size_t i = 0;

    if (str.empty()) {
        throw std::invalid_argument("Input string is empty");
    }

    if (str[i] == '+' || str[i] == '-') {
        sign = (str[i] == '-') ? -1 : 1;
        i++;
    }

    for (; i < str.size(); i++) {
        if (str[i] < '0' || str[i] > '9') {
            throw std::invalid_argument("Invalid character in input string");
        }
        int digit = str[i] - '0';

        if (result > (2147483647 - digit) / 10) {
            throw std::out_of_range("Integer overflow");
        }

        result = result * 10 + digit;
    }

    return result * sign;
}

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

bool isallowed(Client usr, Channel chan, std::string pw) {
    std::string modes = chan.getmode();
    for (size_t i = 0; i < modes.size(); i++) {
        if (modes[i] == 'i' && !chan.isinvited(usr))
            return false;
        if (modes[i] == 'k' && pw != chan.getpw())
            return false;
        if (modes[i] == 'l' && chan.getusercount() > chan.getuserlimit())
            return false;
    }
    return true;
}

// JOIN Command
void join(Client* usr, std::string params, std::vector<Channel>& channels) {
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.empty()) {
        std::ostringstream error;
        error << ":" << hostname << " 461 " << usr->nickname << " JOIN :Not enough parameters\r\n";
        send(usr->socket, error.str().c_str(), error.str().length(), 0);
        return;
    }

    std::string channelName = split[0];
    std::string password = (split.size() > 1) ? split[1] : "";

    for (size_t i = 0; i < channels.size(); i++) {
        if (channels[i].getid() == channelName) {
            if (!isallowed(*usr, channels[i], password)) {
                std::ostringstream error;
                error << ":" << hostname << " 474 " << usr->nickname << " " << channelName << " :Cannot join channel\r\n";
                send(usr->socket, error.str().c_str(), error.str().length(), 0);
                return;
            }

            channels[i].adduser(usr);
            if (channels[i].getusers().size() == 1) {
                channels[i].addoperator(usr);
            }

            std::ostringstream joinMsg;
            joinMsg << ":" << usr->nickname << "!" << usr->username << "@" << usr->hostname << " JOIN " << channelName << "\r\n";
            std::vector<Client*> users = channels[i].getusers();
            for (size_t j = 0; j < users.size(); j++) {
                send(users[j]->socket, joinMsg.str().c_str(), joinMsg.str().length(), 0);
            }

            std::ostringstream topicMsg;
            topicMsg << ":" << hostname << " 332 " << usr->nickname << " " << channelName << " :" << channels[i].gettopic() << "\r\n";
            send(usr->socket, topicMsg.str().c_str(), topicMsg.str().length(), 0);

            std::ostringstream namesMsg;
            namesMsg << ":" << hostname << " 353 " << usr->nickname << " = " << channelName << " :" << channels[i].getnicklist() << "\r\n";
            send(usr->socket, namesMsg.str().c_str(), namesMsg.str().length(), 0);

            std::ostringstream endNamesMsg;
            endNamesMsg << ":" << hostname << " 366 " << usr->nickname << " " << channelName << " :End of /NAMES list\r\n";
            send(usr->socket, endNamesMsg.str().c_str(), endNamesMsg.str().length(), 0);

            return;
        }
    }

    std::ostringstream error;
    error << ":" << hostname << " 403 " << usr->nickname << " " << channelName << " :No such channel\r\n";
    send(usr->socket, error.str().c_str(), error.str().length(), 0);
}

// PART Command
void part(Client* usr, std::string params, std::vector<Channel>& channels) {
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.empty()) {
        std::ostringstream error;
        error << ":" << hostname << " 461 " << usr->nickname << " PART :Not enough parameters\r\n";
        send(usr->socket, error.str().c_str(), error.str().length(), 0);
        return;
    }

    std::string channelName = split[0];
    for (size_t i = 0; i < channels.size(); i++) {
        if (channels[i].getid() == channelName) {
            std::vector<Client*> users = channels[i].getusers();
            bool userFound = false;

            for (size_t j = 0; j < users.size(); j++) {
                if (users[j]->socket == usr->socket) {
                    userFound = true;
                    break;
                }
            }

            if (!userFound) {
                std::ostringstream fail;
                fail << ":" << hostname << " 442 " << usr->nickname << " " << channelName << " :You're not on that channel\r\n";
                send(usr->socket, fail.str().c_str(), fail.str().length(), 0);
                return;
            }

            channels[i].deluser(*usr);

            std::ostringstream partMsg;
            partMsg << ":" << usr->nickname << "!" << usr->username << "@" << usr->hostname << " PART " << channelName << "\r\n";
            std::vector<Client*> userslist = channels[i].getusers();
            for (size_t j = 0; j < userslist.size(); j++) {
                send(userslist[j]->socket, partMsg.str().c_str(), partMsg.str().length(), 0);
            }
            return;
        }
    }

    std::ostringstream fail;
    fail << ":" << hostname << " 403 " << usr->nickname << " " << channelName << " :No such channel\r\n";
    send(usr->socket, fail.str().c_str(), fail.str().length(), 0);
}

// WHO Command
bool who(Client usr, std::string params, std::vector<Channel>& channels) {
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.size() > 1) {
        std::ostringstream error;
        error << ":" << hostname << " 461 " << usr.nickname << " WHO :Not enough parameters\r\n";
        send(usr.socket, error.str().c_str(), error.str().length(), 0);
        return false;
    }

    for (size_t i = 0; i < channels.size(); i++) {
        if (channels[i].getid() == split[0]) {
            std::vector<Client*> users = channels[i].getusers();
            std::ostringstream nicklist;

            for (size_t j = 0; j < users.size(); j++) {
                if (channels[i].isoperator(*users[j])) {
                    nicklist << "@";
                }
                nicklist << users[j]->nickname;
                if (j < users.size() - 1) {
                    nicklist << " ";
                }
            }

            std::ostringstream whobase;
            whobase << ":" << hostname << " 353 " << usr.nickname << " = " << split[0] << " :" << nicklist.str() << "\r\n";
            send(usr.socket, whobase.str().c_str(), whobase.str().length(), 0);

            std::ostringstream whoend;
            whoend << ":" << hostname << " 366 " << usr.nickname << " " << split[0] << " :End of /NAMES list\r\n";
            send(usr.socket, whoend.str().c_str(), whoend.str().length(), 0);

            return true;
        }
    }

    std::ostringstream joinfail;
    joinfail << ":" << hostname << " 403 " << usr.nickname << " " << params << " :No such channel\r\n";
    send(usr.socket, joinfail.str().c_str(), joinfail.str().length(), 0);
    return false;
}

// KICK Command
void kick(Client* usr, std::string params, std::vector<Channel>& channels) {
    std::string hostname = IRCHOSTNAME;
    std::vector<std::string> split = splitString(params, ' ');

    if (split.size() < 2) {
        std::ostringstream error;
        error << ":" << hostname << " 461 " << usr->nickname << " KICK :Not enough parameters\r\n";
        send(usr->socket, error.str().c_str(), error.str().length(), 0);
        return;
    }

    std::string channelName = split[0];
    std::string targetName = split[1];
    std::string reason = (split.size() > 2) ? params.substr(params.find(targetName) + targetName.length() + 1) : "No reason provided";

    for (size_t i = 0; i < channels.size(); i++) {
        if (channels[i].getid() == channelName) {
            if (!channels[i].isoperator(*usr)) {
                std::ostringstream fail;
                fail << ":" << hostname << " 482 " << usr->nickname << " " << channelName << " :You're not channel operator\r\n";
                send(usr->socket, fail.str().c_str(), fail.str().length(), 0);
                return;
            }

            std::vector<Client*> users = channels[i].getusers();
            bool userFound = false;
            for (size_t j = 0; j < users.size(); j++) {
                if (users[j]->nickname == targetName) {
                    userFound = true;

                    std::ostringstream kickMsg;
                    kickMsg << ":" << usr->nickname << "!" << usr->username << "@" << usr->hostname << " KICK " << channelName << " " << targetName << " :" << reason << "\r\n";
                    std::vector<Client*> userslist = channels[i].getusers();
                    for (size_t j = 0; j < userslist.size(); j++) {
                        send(userslist[j]->socket, kickMsg.str().c_str(), kickMsg.str().length(), 0);
            }

                    channels[i].deluser(*users[j]);
                    break;
                }
            }

            if (!userFound) {
                std::ostringstream error;
                error << ":" << hostname << " 441 " << targetName << " " << channelName << " :They aren't on that channel\r\n";
                send(usr->socket, error.str().c_str(), error.str().length(), 0);
            }
            return;
        }
    }

    std::ostringstream error;
    error << ":" << hostname << " 403 " << usr->nickname << " " << channelName << " :No such channel\r\n";
    send(usr->socket, error.str().c_str(), error.str().length(), 0);
}

// Command Handler
bool handle_channel_command(Client* usr, std::string command, std::string params, std::vector<Channel>& channels) {
    if (command == "JOIN") {
        join(usr, params, channels);
        return true;
    } else if (command == "PART") {
        part(usr, params, channels);
        return true;
    } else if (command == "TOPIC") {
        // Implement similar to others
        return true;
    } else if (command == "KICK") {
        kick(usr, params, channels);
        return true;
    } else if (command == "MODE") {
        // Implement as needed
        return true;
    } else if (command == "PRIVMSG") {
        // Implement as needed
        return true;
    }
    return false;
}
