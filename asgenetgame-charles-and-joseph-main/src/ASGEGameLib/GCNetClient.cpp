//
// Created by huxy on 23/02/2020.
//

#include "GCNetClient.hpp"

GCNetClient::GCNetClient() : GameComponent(ID::NETWORK_CLIENT) {}

void GCNetClient::update(double /*dt*/) {}

void GCNetClient::connect(const std::string& server_ip, unsigned short server_port)
{
  socket = (kissnet::endpoint{ server_ip, server_port });
  socket.connect();
}

GCNetClient::~GCNetClient()
{
  socket.close();
}
