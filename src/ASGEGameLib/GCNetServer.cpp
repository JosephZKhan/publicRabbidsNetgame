//
// Created by huxy on 23/02/2020.
//

#include "GCNetServer.hpp"
#include <iostream>
#include <kissnet.hpp>
#include <thread>

namespace
{
  kissnet::tcp_socket bind(const std::string& ip_address, kissnet::port_t port)
  {
    // create endpoint and assigning to a socket
    kissnet::tcp_socket server(kissnet::endpoint(ip_address, port));

    // bind and listen on
    server.bind();
    server.listen();

    // return it
    return server;
  }
} // namespace

GCNetServer::GCNetServer() : GameComponent(ID::NETWORK_SERVER) {}

GCNetServer::~GCNetServer()
{
  accept_connections = false;
  server.close();
}

void GCNetServer::start()
{
  server             = bind("0.0.0.0", 31276);
  accept_connections = true;

  std::thread thread(
    [&]()
    {
      while (accept_connections)
      {
        auto& socket = connections.emplace_back(server.accept());
        std::cout << "connection received from: " << socket.get_recv_endpoint().address << ":"
                  << socket.get_recv_endpoint().port << std::endl;
      }
    });

  thread.detach();
}

void GCNetServer::update(double /*dt*/) {}
