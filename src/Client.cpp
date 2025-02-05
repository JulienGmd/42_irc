#include "Client.hpp"

Client::Client(int socket)
	: socket(socket),
	  has_set_server_password(false),
	  nickname(),
	  username(),
	  hostname()
{
}

bool Client::is_connected()
{
	return socket != -1;
}