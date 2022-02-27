//
// Created by james on 28/01/2021.
//

#ifndef ASGENETGAME_ASGESERVER_HPP
#define ASGENETGAME_ASGESERVER_HPP

#include "atomic"
#include "iostream"
#include "kissnet.hpp"
#include "list"
#include "thread"
#include "vector"
#include <ASGEGameLib/GCNetServer.hpp>

namespace
{
  constexpr kissnet::port_t port = 12321;
  std::atomic running            = true;
  using socket_cref              = std::reference_wrapper<const kissnet::tcp_socket>;
  using socket_list              = std::list<socket_cref>;
}

class ASGEServer
{
 public:
  ASGEServer()  = default;
  ~ASGEServer() = default;

  void listen(kissnet::tcp_socket& socket);
  void run(const std::string& listen_address, kissnet::port_t listen_port);
  void send(const kissnet::buffer<4096>&, size_t length, const socket_list&);
  bool init();
  int run();

 private:
  GCNetServer net_server;
};

#endif // ASGENETGAME_ASGESERVER_HPP
