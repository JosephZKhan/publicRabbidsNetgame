//
// Created by james on 28/01/2021.
//

#include "ASGEServer.hpp"
#include "atomic"
#include <iostream>

#include "list"

#include "thread"

#include "vector"
namespace
{
  std::list<kissnet::tcp_socket>& connections()
  {
    auto static connections_ = new std::list<kissnet::tcp_socket>;
    return *connections_;
  }

  std::vector<std::thread>& workers()
  {
    auto static workers_ = new std::vector<std::thread>;
    return *workers_;
  }
}

bool ASGEServer::init()
{
  net_server.start();
  return true;
}

void ASGEServer::listen(kissnet::tcp_socket& socket)
{
  // keep client socket active until connection is no longer valid
  bool continue_receiving = true;
  // we will read a max of 4K at any one time
  kissnet::buffer<4096> static_buffer;
  while (continue_receiving)
  {
    // is there data ready for us?
    if (auto [size, valid] = socket.recv(static_buffer); valid)
    {
      if (valid.value == kissnet::socket_status::cleanly_disconnected)
      {
        continue_receiving = false; // client disconnected
      }

      // add a null terminator, and print as string

      if (size < static_buffer.size())
        static_buffer[size] = std::byte{ 0 };

      std::cout << reinterpret_cast<char*>(static_buffer.data()) << '\n';
      send(static_buffer, size, { socket });
    }
    // if not valid remote host closed connection
    else
    {
      continue_receiving = false;
      socket.close();
    }
  }
}

void ASGEServer::run(const std::string& listen_address, kissnet::port_t listen_port)
{
  kissnet::tcp_socket server(kissnet::endpoint(listen_address, listen_port));
  server.bind();   // bind the server to the endpoint
  server.listen(); // set the socket to listen

  // loop that will continuously accept connections
  while (running)
  {
    auto& socket = connections().emplace_back(server.accept());
    std::cout << "detected connection: " << socket.get_recv_endpoint().address << ":"
              << socket.get_recv_endpoint().port << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    workers().emplace_back(
      [&]
      {
        // listens for messages on the connected socket
        listen(socket);
        // we know the socket disconnected, so remove it from the list
        std::cout << "detected disconnect\n";
        if (const auto socket_iter =
              std::find(connections().begin(), connections().end(), std::ref(socket));
            socket_iter != connections().end())
        {
          std::cout << "closing socket...\n";
          connections().erase(socket_iter);
        }
      });
    // this lets the run asynchronously

    workers().back().detach();
  }
  server.close(); // terminate the server socket
}

void ASGEServer::send(
  const kissnet::buffer<4096>& buffer, size_t length, const socket_list& exclude = {})
{
  for (auto& socket : connections())
  {
    if (auto it = std::find(exclude.cbegin(), exclude.cend(), socket); it == exclude.cend())
    {
      socket.send(buffer, length);
    }
  }
}

int ASGEServer::run()
{
  run("0.0.0.0", port);
  delete &connections();
  delete &workers();
  return EXIT_SUCCESS;
}
