#include "ASGEGame.hpp"
#include <Engine/Logger.hpp>

int main(int /*argc*/, char* /*argv*/[])
{
  ASGE::GameSettings game_settings;
  game_settings.window_title  = "ASGENetGame";
  game_settings.window_width  = 720;
  game_settings.window_height = 720;
  game_settings.mode          = ASGE::GameSettings::WindowMode::WINDOWED;
  game_settings.fixed_ts      = 30;
  game_settings.fps_limit     = 120;
  game_settings.msaa_level    = 16;

  client Net;

  auto socket = Net.connect("192.168.0.42", 12321);
  Net.run();

  Logging::INFO("Launching Game!");
  ASGENetGame game(game_settings, Net, socket);
  game.run();
  return 0;
}
