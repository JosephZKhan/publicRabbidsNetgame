//
// Created by james on 28/01/2021.
//

#include "ASGEServer.hpp"
int main(int /*argc*/, char* /*argv*/[])
{
  ASGEServer server{};
  if (server.init())
  {
    return server.run();
  }
}
