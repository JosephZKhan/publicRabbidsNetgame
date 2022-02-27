//
// Created by Charles on 30/01/2022.
//

#include "client.h"
#include "atomic"
#include "iostream"
#include "kissnet.hpp"
#include "thread"
#include <fstream>

// function prototypes

kissnet::tcp_socket client::connect(const std::string& server_address, kissnet::port_t port)
{
  // attempt connection to server and return the connected socket
  kissnet::tcp_socket client_socket(kissnet::endpoint{ server_address, port });
  client_socket.connect(0);
  connected = true;
  return client_socket;
}

void client::send(kissnet::tcp_socket& socket, std::string x) const
{
  if (connected && running)
  {
    socket.send(reinterpret_cast<std::byte*>(x.data()), x.length());
  }
}

long long client::createUniqueID()
{
  // now ideally i would take the id's from diffrent pieces of hardware and the MAC address of the
  // network card to create a hash that only that machine would have, however due to the small
  // playerbase this project will have we are going to use a random number.

  long long int x;
  x = std::rand() * std::rand() * std::rand();
  std::ofstream out("uniqueid.idFile");
  out << x;
  out.close();
  return x;
}

long long client::getUniqueID()
{
  std::ifstream Unq;
  long long int x = 0;
  long long int y = 0;
  Unq.open("uniqueid.idFile");
  if (!Unq)
  {
    return client::createUniqueID();
  }
  else
  {
    while (Unq >> x)
    {
      y = y + x;
    }
    Unq.close();
    std::cout << y << std::endl;
    return y;
  }
}

void client::run()
{
  running = true;
}