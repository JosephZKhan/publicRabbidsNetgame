#ifndef ASGENETGAME_CLIENT_H
#define ASGENETGAME_CLIENT_H
#include "kissnet.hpp"
class client
{
 public:
  bool connected = false;
  bool running   = false;

  kissnet::tcp_socket connect(const std::string& server_address, kissnet::port_t port);
  void send(kissnet::tcp_socket& socket, std::string x) const;
  void run();
  long long int createUniqueID();
  long long int getUniqueID();
  long long int ClientID;
};

#endif // ASGENETGAME_CLIENT_H