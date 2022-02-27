//
// Created by huxy on 23/02/2020.
//

#pragma once

#include "Block.h"
#include "Player.h"
#include "Projectile.h"
#include "client.h"
#include "kissnet.hpp"
#include <ASGEGameLib/GComponent.hpp>
#include <Engine/OGLGame.hpp>
#include <Engine/Sprite.hpp>
#include <tmxlite/Layer.hpp>
#include <tmxlite/Map.hpp>
#include <tmxlite/TileLayer.hpp>
#include <vector>

class ASGENetGame : public ASGE::OGLGame
{
 public:
  explicit ASGENetGame(
    const ASGE::GameSettings& settings, client& net, kissnet::tcp_socket& netSocket);
  ~ASGENetGame() override;

  ASGENetGame(const ASGENetGame&) = delete;
  ASGENetGame& operator=(const ASGENetGame&) = delete;

  void keyHandler(ASGE::SharedEventData data);
  void clickHandler(ASGE::SharedEventData data);
  void update(const ASGE::GameTime& us) override;
  void render(const ASGE::GameTime& us) override;
  void fixedUpdate(const ASGE::GameTime& us) override;

  client& Net;
  kissnet::tcp_socket& NetSocket;
  void clientlisten(kissnet::tcp_socket& socket);

 private:
  bool initPlayers();
  bool initProjectiles();
  void initFont();
  void movePlayer(unsigned int playerIndex, float moveUnitsX, float moveUnitsY, bool team);
  void moveProjectile(float moveUnitsX, float moveUnitsY);
  bool
  directionalCollisionCheck(const ASGE::Sprite* target, std::array<ASGE::Point2D, 2> collPoints);
  void shoot(unsigned int playerIndex, bool team);
  std::vector<std::unique_ptr<GameComponent>> game_components;
  std::string key_callback_id{};   /**< Key Input Callback ID. */
  std::string mouse_callback_id{}; /**< Mouse Input Callback ID. */
  std::vector<std::unique_ptr<ASGE::Sprite>> tiles;
  std::vector<std::unique_ptr<ASGE::Sprite>> collidables;
  std::vector<Player> team1{ 4 };
  std::vector<Player> team2{ 4 };
  bool clickedOnPLayer;
  ASGE::Point2D playerMoveUnits;
  float lerpFactor = 20;
  float lerpIndex  = 0;
  std::vector<Projectile> projectiles{ 9 };
  unsigned int projectileIndex = 0;
  ASGE::Point2D projectileMoveUnits;
  float projectileLerpFactor = 25;
  float projectileLerpIndex  = 0;
  bool isBlueTeam            = false;
  bool isMyTurn              = true;
  std::unique_ptr<ASGE::Sprite> lobbyConnectButton;
  bool inLobby             = true;
  bool inGame              = false;
  int fontIdx              = 0;
  ASGE::Text turnIndicator = {};
};
