//
// Created by huxy on 23/02/2020.
//

#ifndef NETGAME_GCNETCLIENT_HPP
#define NETGAME_GCNETCLIENT_HPP
#include "GComponent.hpp"
#include <kissnet.hpp>
#include <string>

class GCNetClient : public GameComponent
{
 public:
  GCNetClient();
  ~GCNetClient() override;
  void connect(const std::string& server_ip, unsigned short server_port);
  void update(double dt) override;

  GCNetClient(const GCNetClient&) = delete;
  GCNetClient& operator=(const GCNetClient&) = delete;

 private:
  kissnet::tcp_socket socket;
};

#endif // NETGAME_GCNETCLIENT_HPP
