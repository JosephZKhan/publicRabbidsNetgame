#include "ASGEGame.hpp"
#include "atomic"
#include "client.h"
#include "iostream"
#include "kissnet.hpp"
#include "regex"
#include "thread"
#include <ASGEGameLib/GCNetClient.hpp>
#include <Engine/FileIO.hpp>
#include <fstream>
#include <iostream>

/// initialises the game.
/// Setup your game and initialise the core components.
/// @param settings
ASGENetGame::ASGENetGame(
  const ASGE::GameSettings& settings, client& net, kissnet::tcp_socket& netSocket) :
  OGLGame(settings),
  Net(net), NetSocket(netSocket)
{
  key_callback_id   = inputs->addCallbackFnc(ASGE::E_KEY, &ASGENetGame::keyHandler, this);
  mouse_callback_id = inputs->addCallbackFnc(ASGE::E_MOUSE_CLICK, &ASGENetGame::clickHandler, this);
  inputs->use_threads = false;
  std::thread listen_thread([&] { clientlisten(NetSocket); });
  listen_thread.detach();
  // game_components.emplace_back(std::move(client));

  // socket.send(reinterpret_cast<std::byte*>(str.data()), str.length());
  ASGE::FILEIO::File tileMap;
  if (tileMap.open("data/map/gameTileMap.tmx"))
  {
    ASGE::FILEIO::IOBuffer buffer = tileMap.read();
    std::string fileString(buffer.as_char(), buffer.length);

    tmx::Map map;
    map.loadFromString(fileString, ".");

    for (const auto& layer : map.getLayers())
    {
      if (layer->getType() == tmx::Layer::Type::Tile)
      {
        auto tileLayer = layer->getLayerAs<tmx::TileLayer>();
        for (unsigned int row = 0; row < layer->getSize().y; ++row)
        {
          for (unsigned int col = 0; col < layer->getSize().x; ++col)
          {
            auto tileInfo = tileLayer.getTiles()[row * tileLayer.getSize().x + col];
            auto tile     = map.getTilesets()[0].getTile(tileInfo.ID);
            if (tile != nullptr)
            {
              auto& sprite = tiles.emplace_back(renderer->createUniqueSprite());

              if (sprite->loadTexture(tile->imagePath))
              {
                sprite->srcRect()[0] = static_cast<float>(tile->imagePosition.x);
                sprite->srcRect()[1] = static_cast<float>(tile->imagePosition.y);
                sprite->srcRect()[2] = static_cast<float>(tile->imageSize.x);
                sprite->srcRect()[3] = static_cast<float>(tile->imageSize.y);

                sprite->width(static_cast<float>(tile->imageSize.x));
                sprite->height(static_cast<float>(tile->imageSize.y));

                sprite->scale(static_cast<float>(1.4));
                sprite->setMagFilter(ASGE::Texture2D::MagFilter::NEAREST);

                sprite->yPos(static_cast<float>(row * tile->imageSize.y * 1.4));
                sprite->xPos(static_cast<float>(col * tile->imageSize.x * 1.4));
              }
            }
          }
        }
      }

      if (layer->getName() == "breakables" || layer->getName() == "unbreakables")
      {
        if (layer->getType() == tmx::Layer::Type::Tile)
        {
          auto tileLayer = layer->getLayerAs<tmx::TileLayer>();
          for (unsigned int row = 0; row < layer->getSize().y; ++row)
          {
            for (unsigned int col = 0; col < layer->getSize().x; ++col)
            {
              auto tileInfo = tileLayer.getTiles()[row * tileLayer.getSize().x + col];
              auto tile     = map.getTilesets()[0].getTile(tileInfo.ID);
              if (tile != nullptr)
              {
                auto& sprite = collidables.emplace_back(renderer->createUniqueSprite());
                if (sprite->loadTexture(tile->imagePath))
                {
                  sprite->srcRect()[0] = static_cast<float>(tile->imagePosition.x);
                  sprite->srcRect()[1] = static_cast<float>(tile->imagePosition.y);
                  sprite->srcRect()[2] = static_cast<float>(tile->imageSize.x);
                  sprite->srcRect()[3] = static_cast<float>(tile->imageSize.y);

                  sprite->width(static_cast<float>(tile->imageSize.x * 1.4));
                  sprite->height(static_cast<float>(tile->imageSize.y * 1.4));

                  sprite->scale(3);
                  sprite->setMagFilter(ASGE::Texture2D::MagFilter::NEAREST);

                  sprite->yPos(static_cast<float>(row * tile->imageSize.y * 1.4));
                  sprite->xPos(static_cast<float>(col * tile->imageSize.x * 1.4));
                }
              }
            }
          }
        }
      }
    }
  }

  initPlayers();
  initProjectiles();
  initFont();

  lobbyConnectButton = renderer->createUniqueSprite();
  if (!lobbyConnectButton->loadTexture("/data/img/icons/lobbyConnectButton.png"))
  {
    std::cout << "error - failed to load connect button sprite\n";
  }
  lobbyConnectButton->width(243);
  lobbyConnectButton->height(150);

  lobbyConnectButton->scale(static_cast<float>(1));
  lobbyConnectButton->setMagFilter(ASGE::Texture2D::MagFilter::NEAREST);

  lobbyConnectButton->xPos((720 / 2) - (lobbyConnectButton->width() / 2));
  lobbyConnectButton->yPos(400);

  // game_components.emplace_back(std::move(client));
}

/// Destroys the game.
ASGENetGame::~ASGENetGame()
{
  this->inputs->unregisterCallback(key_callback_id);
  this->inputs->unregisterCallback(mouse_callback_id);
}

/// Processes key inputs.
/// This function is added as a callback to handle the game's
/// keyboard input. For this game, calls to this function
/// are not thread safe, so you may alter the game's state
/// but your code needs to designed to prevent data-races.
/// @param data
/// @see KeyEvent
void ASGENetGame::keyHandler(ASGE::SharedEventData data)
{
  const auto* key = dynamic_cast<const ASGE::KeyEvent*>(data.get());

  if (key->key == ASGE::KEYS::KEY_ESCAPE)
  {
    signalExit();
  }
}

void ASGENetGame::clickHandler(ASGE::SharedEventData data)
{
  const auto* click = dynamic_cast<const ASGE::ClickEvent*>(data.get());

  // std::string str{ "12~34~56~78~99" };
  // Net.send(NetSocket, str);

  if (click->action == ASGE::MOUSE::BUTTON_PRESSED)
  {
    std::cout << "click position: " << click->xpos << " " << click->ypos << "\n";

    if (inLobby)
    {
      if (click->xpos > lobbyConnectButton->xPos())
      {
        if (click->xpos < lobbyConnectButton->xPos() + lobbyConnectButton->width())
        {
          if (click->ypos > lobbyConnectButton->yPos())
          {
            if (click->ypos < lobbyConnectButton->yPos() + lobbyConnectButton->height())
            {
              Net.send(
                NetSocket,
                std::string("4~") + std::to_string(1) + "~" + std::to_string(00) + "~" +
                  std::to_string(00) + "~00");
              // inLobby = false;
              // inGame  = true;
            }
          }
        }
      }
    }

    if (!isMyTurn)
      return;
    if (!inGame)
      return;

    for (unsigned int i = 0; i <= 3; ++i)
    {
      if (isBlueTeam)
      {
        if (team1[i].getIsSelected())
        {
          if (click->xpos > team1[i].movementRangeSprite->xPos())
          {
            if (
              click->xpos <
              team1[i].movementRangeSprite->xPos() + (team1[i].movementRangeSprite->width() * 2))
            {
              if (click->ypos > team1[i].movementRangeSprite->yPos())
              {
                if (
                  click->ypos < team1[i].movementRangeSprite->yPos() +
                                  (team1[i].movementRangeSprite->height() * 2))
                {
                  team1[i].setIsMoving(true);
                  playerMoveUnits.x = static_cast<float>(
                    click->xpos -
                    (team1[i].sprite->xPos() + ((team1[i].sprite->width() * 1.4) / 2)));
                  playerMoveUnits.y = static_cast<float>(
                    click->ypos -
                    (team1[i].sprite->yPos() + ((team1[i].sprite->height() * 1.4) / 2)));
                }
              }
            }
          }
        }
        team1[i].deselect();
        if (click->xpos > team1[i].sprite->xPos())
        {
          if (click->xpos < team1[i].sprite->xPos() + (team1[i].sprite->width() * 1.4))
          {
            if (click->ypos > team1[i].sprite->yPos())
            {
              if (click->ypos < team1[i].sprite->yPos() + (team1[i].sprite->height() * 1.4))
              {
                clickedOnPLayer = true;
              }
            }
          }
        }
        if (team1[i].getInShootingMode())
        {
          if (click->xpos > team1[i].shootingRangeSprite->xPos())
          {
            if (
              click->xpos <
              team1[i].shootingRangeSprite->xPos() + (team1[i].shootingRangeSprite->width() * 2))
            {
              if (click->ypos > team1[i].shootingRangeSprite->yPos())
              {
                if (
                  click->ypos < team1[i].shootingRangeSprite->yPos() +
                                  (team1[i].shootingRangeSprite->height() * 2))
                {
                  projectileMoveUnits.x = static_cast<float>(
                    click->xpos -
                    (team1[i].sprite->xPos() + ((team1[i].sprite->width() * 1.4) / 2)));
                  projectileMoveUnits.y = static_cast<float>(
                    click->ypos -
                    (team1[i].sprite->yPos() + ((team1[i].sprite->height() * 1.4) / 2)));
                  shoot(i, isBlueTeam);
                }
              }
            }
          }
        }
      }
      else
      {
        if (team2[i].getIsSelected())
        {
          if (click->xpos > team2[i].movementRangeSprite->xPos())
          {
            if (
              click->xpos <
              team2[i].movementRangeSprite->xPos() + (team2[i].movementRangeSprite->width() * 2))
            {
              if (click->ypos > team2[i].movementRangeSprite->yPos())
              {
                if (
                  click->ypos < team2[i].movementRangeSprite->yPos() +
                                  (team2[i].movementRangeSprite->height() * 2))
                {
                  team2[i].setIsMoving(true);
                  playerMoveUnits.x = static_cast<float>(
                    click->xpos -
                    (team2[i].sprite->xPos() + ((team2[i].sprite->width() * 1.4) / 2)));
                  playerMoveUnits.y = static_cast<float>(
                    click->ypos -
                    (team2[i].sprite->yPos() + ((team2[i].sprite->height() * 1.4) / 2)));
                }
              }
            }
          }
        }
        team2[i].deselect();
        if (click->xpos > team2[i].sprite->xPos())
        {
          if (click->xpos < team2[i].sprite->xPos() + (team2[i].sprite->width() * 1.4))
          {
            if (click->ypos > team2[i].sprite->yPos())
            {
              if (click->ypos < team2[i].sprite->yPos() + (team2[i].sprite->height() * 1.4))
              {
                clickedOnPLayer = true;
              }
            }
          }
        }
        if (team2[i].getInShootingMode())
        {
          if (click->xpos > team2[i].shootingRangeSprite->xPos())
          {
            if (
              click->xpos <
              team2[i].shootingRangeSprite->xPos() + (team2[i].shootingRangeSprite->width() * 2))
            {
              if (click->ypos > team2[i].shootingRangeSprite->yPos())
              {
                if (
                  click->ypos < team2[i].shootingRangeSprite->yPos() +
                                  (team2[i].shootingRangeSprite->height() * 2))
                {
                  projectileMoveUnits.x = static_cast<float>(
                    click->xpos -
                    (team2[i].sprite->xPos() + ((team2[i].sprite->width() * 1.4) / 2)));
                  projectileMoveUnits.y = static_cast<float>(
                    click->ypos -
                    (team2[i].sprite->yPos() + ((team2[i].sprite->height() * 1.4) / 2)));
                  shoot(i, isBlueTeam);
                }
              }
            }
          }
        }
      }
    }
  }

  if (clickedOnPLayer)
  {
    if (click->action == ASGE::MOUSE::BUTTON_RELEASED)
    {
      clickedOnPLayer = false;
      for (unsigned int i = 0; i <= 3; ++i)
      {
        if (isBlueTeam)
        {
          if (click->xpos > team1[i].sprite->xPos())
          {
            if (click->xpos < team1[i].sprite->xPos() + (team1[i].sprite->width() * 1.4))
            {
              if (click->ypos > team1[i].sprite->yPos())
              {
                if (click->ypos < team1[i].sprite->yPos() + (team1[i].sprite->height() * 1.4))
                {
                  team1[i].select();
                }
              }
            }
          }
        }
        else
        {
          if (click->xpos > team2[i].sprite->xPos())
          {
            if (click->xpos < team2[i].sprite->xPos() + (team2[i].sprite->width() * 1.4))
            {
              if (click->ypos > team2[i].sprite->yPos())
              {
                if (click->ypos < team2[i].sprite->yPos() + (team2[i].sprite->height() * 1.4))
                {
                  team2[i].select();
                }
              }
            }
          }
        }
      }
    }
  }
}

/// Updates your game and all it's components.
/// @param us
void ASGENetGame::update(const ASGE::GameTime& us)
{
  for (auto& gc : game_components)
  {
    gc->update(us.deltaInSecs());
  }

  for (unsigned int k = 0; k <= 8; ++k)
  {
    if (projectiles[k].getIsActive())
    {
      moveProjectile(projectileMoveUnits.x, projectileMoveUnits.y);
    }
  }

  // net

  for (unsigned int i = 0; i <= 3; ++i)
  {
    if (!team1[i].getIsAlive() and !team1[i].serverKnows)
    {
      team1[i].serverKnows = true;
      Net.send(
        NetSocket,
        std::string("3~") + std::to_string(i) + "~" + std::to_string(1) + "~" + std::to_string(00) +
          "~00");
    }
    if (!team2[i].getIsAlive() and !team2[i].serverKnows)
    {
      team2[i].serverKnows = true;
      Net.send(
        NetSocket,
        std::string("3~") + std::to_string(i) + "~" + std::to_string(2) + "~" + std::to_string(0) +
          "~00");
    }
  }

  for (unsigned int i = 0; i <= 3; ++i)
  {
    if (isBlueTeam)
    {
      if (team1[i].getIsSelected())
      {
        std::cout << "unit " << i + 1 << " selected!\n";
        team1[i].setRenderMovementSprite(true);
      }
      else
      {
        team1[i].setRenderMovementSprite(false);
      }
      if (team1[i].getIsMoving())
      {
        movePlayer(i, playerMoveUnits.x, playerMoveUnits.y, isBlueTeam);
      }
      if (team1[i].getInShootingMode())
      {
        team1[i].setRenderShootingSprite(true);
      }
      else
      {
        team1[i].setRenderShootingSprite(false);
      }

      if (team1[i].getHealth() <= 0)
      {
        team1[i].setIsAlive(false);
      }

      if (team2[i].getHealth() <= 0)
      {
        team2[i].setIsAlive(false);
      }
    }
    else
    {
      if (team2[i].getIsSelected())
      {
        std::cout << "unit " << i + 1 << " selected!\n";
        team2[i].setRenderMovementSprite(true);
      }
      else
      {
        team2[i].setRenderMovementSprite(false);
      }
      if (team2[i].getIsMoving())
      {
        /*std::cout << "move " << playerMoveUnits.x << " units right and " << playerMoveUnits.y
                  << " units down.\n";*/
        movePlayer(i, playerMoveUnits.x, playerMoveUnits.y, isBlueTeam);
      }
      if (team2[i].getInShootingMode())
      {
        //      std::cout << "unit " << i + 1 << " is in shooting mode.\n";
        team2[i].setRenderShootingSprite(true);
      }
      else
      {
        team2[i].setRenderShootingSprite(false);
      }

      if (team2[i].getHealth() <= 0)
      {
        team2[i].setIsAlive(false);
      }

      if (team1[i].getHealth() <= 0)
      {
        team1[i].setIsAlive(false);
      }
    }
  }

  int aliveBluePlayers   = 0;
  int aliveYellowPlayers = 0;

  for (unsigned int i = 0; i <= 3; ++i)
  {
    if (team1[i].getIsAlive())
    {
      aliveBluePlayers++;
    }
    if (team2[i].getIsAlive())
    {
      aliveYellowPlayers++;
    }
  }

  if (isBlueTeam)
  {
    if (aliveBluePlayers <= 0)
    {
      inGame = false;
      std::cout << "YOU LOSE!\n";
    }
    if (aliveYellowPlayers <= 0)
    {
      inGame = false;
      std::cout << "YOU WIN!\n";
    }
  }
  else
  {
    if (aliveYellowPlayers <= 0)
    {
      inGame = false;
      std::cout << "YOU LOSE!\n";
    }
    if (aliveBluePlayers <= 0)
    {
      inGame = false;
      std::cout << "YOU WIN!\n";
    }
  }
}

/// Render your game and its scenes here.
void ASGENetGame::render(const ASGE::GameTime& /*us*/)
{
  if (inLobby)
  {
    renderer->render(*lobbyConnectButton);
  }

  if (inGame)
  {
    for (const auto& i : tiles)
    {
      renderer->render(*i);
    }

    for (unsigned int i = 0; i <= 3; ++i)
    {
      if (isBlueTeam)
      {
        if (team1[i].getIsAlive())
        {
          renderer->render(*team1[i].sprite);
        }

        if (team1[i].getRenderMovementSprite())
        {
          renderer->render(*team1[i].movementRangeSprite);
        }
        if (team1[i].getRenderShootingSprite())
        {
          renderer->render(*team1[i].shootingRangeSprite);
        }

        for (unsigned int j = 0; j <= 3; ++j)
        {
          if (team2[i].sprite->xPos() >= team1[j].shootingRangeSprite->xPos())
          {
            if (
              team2[i].sprite->xPos() + (team2[i].sprite->width() * 1.4) <=
              team1[j].shootingRangeSprite->xPos() + (team1[j].shootingRangeSprite->width() * 2))
            {
              if (team2[i].sprite->yPos() >= team1[j].shootingRangeSprite->yPos())
              {
                if (
                  team2[i].sprite->yPos() + (team2[i].sprite->height() * 1.4) <=
                  team1[j].shootingRangeSprite->yPos() +
                    (team1[j].shootingRangeSprite->height() * 2))
                {
                  if (team2[i].getIsAlive())
                  {
                    renderer->render(*team2[i].sprite);
                  }
                }
              }
            }
          }
        }
      }
      else
      {
        if (team2[i].getIsAlive())
        {
          renderer->render(*team2[i].sprite);
        }

        if (team2[i].getRenderMovementSprite())
        {
          renderer->render(*team2[i].movementRangeSprite);
        }
        if (team2[i].getRenderShootingSprite())
        {
          renderer->render(*team2[i].shootingRangeSprite);
        }

        for (unsigned int j = 0; j <= 3; ++j)
        {
          if (team1[i].sprite->xPos() >= team2[j].shootingRangeSprite->xPos())
          {
            if (
              team1[i].sprite->xPos() + (team1[i].sprite->width() * 1.4) <=
              team2[j].shootingRangeSprite->xPos() + (team2[j].shootingRangeSprite->width() * 2))
            {
              if (team1[i].sprite->yPos() >= team2[j].shootingRangeSprite->yPos())
              {
                if (
                  team1[i].sprite->yPos() + (team1[i].sprite->height() * 1.4) <=
                  team2[j].shootingRangeSprite->yPos() +
                    (team2[j].shootingRangeSprite->height() * 2))
                {
                  if (team1[i].getIsAlive())
                  {
                    renderer->render(*team1[i].sprite);
                  }
                }
              }
            }
          }
        }
      }
    }

    for (unsigned int k = 0; k <= 8; ++k)
    {
      if (projectiles[k].getIsActive())
      {
        renderer->render(*projectiles[k].sprite);
      }
    }

    if (isMyTurn)
    {
      renderer->render(turnIndicator);
    }
  }
}

/// Calls to fixedUpdate use the same fixed timestep
/// irrespective of how much time is passed. If you want
/// deterministic behaviour on clients, this is the place
/// to go.
///
/// https://gamedev.stackexchange.com/questions/1589/when-should-i-use-a-fixed-or-variable-time-step
/// "Use variable timesteps for your game and fixed steps for physics"
/// @param us
void ASGENetGame::fixedUpdate(const ASGE::GameTime& us)
{
  Game::fixedUpdate(us);
}

bool ASGENetGame::initPlayers()
{
  // create sprites for both teams
  for (unsigned int i = 0; i <= 3; ++i)
  {
    team1[i].sprite = renderer->createUniqueSprite();
    team2[i].sprite = renderer->createUniqueSprite();
  }

  // initialize individual sprite textures
  // team 1
  if (!team1[0].sprite->loadTexture("/data/img/playerSprites/player1A.png"))
  {
    // report error message if texture load fails
    std::cout << "error - failed to load unit 1A\n";
    return false;
  }

  if (!team1[1].sprite->loadTexture("/data/img/playerSprites/player1B.png"))
  {
    // report error message if texture load fails
    std::cout << "error - failed to load unit 1B\n";
    return false;
  }

  if (!team1[2].sprite->loadTexture("/data/img/playerSprites/player1C.png"))
  {
    // report error message if texture load fails
    std::cout << "error - failed to load unit 1C\n";
    return false;
  }

  if (!team1[3].sprite->loadTexture("/data/img/playerSprites/player1D.png"))
  {
    // report error message if texture load fails
    std::cout << "error - failed to load unit 1D\n";
    return false;
  }

  // team 2

  if (!team2[0].sprite->loadTexture("/data/img/playerSprites/player2A.png"))
  {
    // report error message if texture load fails
    std::cout << "error - failed to load unit 2A\n";
    return false;
  }

  if (!team2[1].sprite->loadTexture("/data/img/playerSprites/player2B.png"))
  {
    // report error message if texture load fails
    std::cout << "error - failed to load unit 2B\n";
    return false;
  }

  if (!team2[2].sprite->loadTexture("/data/img/playerSprites/player2C.png"))
  {
    // report error message if texture load fails
    std::cout << "error - failed to load unit 2C\n";
    return false;
  }

  if (!team2[3].sprite->loadTexture("/data/img/playerSprites/player2D.png"))
  {
    // report error message if texture load fails
    std::cout << "error - failed to load unit 2D\n";
    return false;
  }

  // set unit positions
  // team 1
  team1[0].sprite->xPos(60);
  team1[0].sprite->yPos(100);

  team1[1].sprite->xPos(200);
  team1[1].sprite->yPos(75);

  team1[2].sprite->xPos(75);
  team1[2].sprite->yPos(200);

  team1[3].sprite->xPos(300);
  team1[3].sprite->yPos(100);

  // team 2

  team2[0].sprite->xPos(640);
  team2[0].sprite->yPos(600);

  team2[1].sprite->xPos(500);
  team2[1].sprite->yPos(625);

  team2[2].sprite->xPos(625);
  team2[2].sprite->yPos(500);

  team2[3].sprite->xPos(400);
  team2[3].sprite->yPos(600);

  for (unsigned int i = 0; i <= 3; ++i)
  {
    team1[i].sprite->width(16);
    team1[i].sprite->height(16);
    team1[i].sprite->scale(static_cast<float>(1.4));
    team1[i].sprite->setMagFilter(ASGE::GameSettings::MagFilter::NEAREST);

    team2[i].sprite->width(16);
    team2[i].sprite->height(16);
    team2[i].sprite->scale(static_cast<float>(1.4));
    team2[i].sprite->setMagFilter(ASGE::GameSettings::MagFilter::NEAREST);
  }

  for (unsigned int i = 0; i <= 3; ++i)
  {
    team1[i].movementRangeSprite = renderer->createUniqueSprite();

    if (!team1[i].movementRangeSprite->loadTexture("/data/img/icons/playerMovementRange.png"))
    {
      std::cout << "error - movement range sprite failed to load - team 1\n";
      return false;
    }

    team1[i].movementRangeSprite->xPos(team1[i].sprite->xPos() - 85);
    team1[i].movementRangeSprite->yPos(team1[i].sprite->yPos() - 85);
    team1[i].movementRangeSprite->width(100);
    team1[i].movementRangeSprite->height(100);
    team1[i].movementRangeSprite->scale(static_cast<float>(2));
    team1[i].movementRangeSprite->setMagFilter(ASGE::GameSettings::MagFilter::NEAREST);

    team2[i].movementRangeSprite = renderer->createUniqueSprite();

    if (!team2[i].movementRangeSprite->loadTexture("/data/img/icons/playerMovementRange.png"))
    {
      std::cout << "error - movement range sprite failed to load - team 2\n";
      return false;
    }

    team2[i].movementRangeSprite->xPos(team2[i].sprite->xPos() - 85);
    team2[i].movementRangeSprite->yPos(team2[i].sprite->yPos() - 85);
    team2[i].movementRangeSprite->width(100);
    team2[i].movementRangeSprite->height(100);
    team2[i].movementRangeSprite->scale(static_cast<float>(2));
    team2[i].movementRangeSprite->setMagFilter(ASGE::GameSettings::MagFilter::NEAREST);
  }

  for (unsigned int i = 0; i <= 3; ++i)
  {
    team1[i].shootingRangeSprite = renderer->createUniqueSprite();

    if (!team1[i].shootingRangeSprite->loadTexture("/data/img/icons/playerShootingRange.png"))
    {
      std::cout << "error - shooting range sprite failed to load - team 1\n";
      return false;
    }

    team1[i].shootingRangeSprite->xPos(team1[i].sprite->xPos() - 180);
    team1[i].shootingRangeSprite->yPos(team1[i].sprite->yPos() - 180);
    team1[i].shootingRangeSprite->width(200);
    team1[i].shootingRangeSprite->height(200);
    team1[i].shootingRangeSprite->scale(static_cast<float>(2));
    team1[i].shootingRangeSprite->setMagFilter(ASGE::GameSettings::MagFilter::NEAREST);

    team2[i].shootingRangeSprite = renderer->createUniqueSprite();

    if (!team2[i].shootingRangeSprite->loadTexture("/data/img/icons/playerShootingRange.png"))
    {
      std::cout << "error - shooting range sprite failed to load - team 2\n";
      return false;
    }

    team2[i].shootingRangeSprite->xPos(team2[i].sprite->xPos() - 180);
    team2[i].shootingRangeSprite->yPos(team2[i].sprite->yPos() - 180);
    team2[i].shootingRangeSprite->width(200);
    team2[i].shootingRangeSprite->height(200);
    team2[i].shootingRangeSprite->scale(static_cast<float>(2));
    team2[i].shootingRangeSprite->setMagFilter(ASGE::GameSettings::MagFilter::NEAREST);
  }

  return true;
}

void ASGENetGame::movePlayer(unsigned int playerIndex, float moveUnitsX, float moveUnitsY, bool team)
{
  if (team)
  {
    float a = (moveUnitsX / lerpFactor);
    float b = (moveUnitsY / lerpFactor);

    if (lerpIndex <= lerpFactor)
    {
      team1[playerIndex].setCanMoveInX(true);

      float newXPos = team1[playerIndex].sprite->xPos() + a;

      ASGE::Point2D newTopRight;
      ASGE::Point2D newBottomRight;
      ASGE::Point2D newTopLeft;
      ASGE::Point2D newBottomLeft;

      newTopRight    = { static_cast<float>(newXPos + (team1[playerIndex].sprite->width() * 1.4)),
                      team1[playerIndex].sprite->yPos() };
      newBottomRight = { static_cast<float>(newXPos + (team1[playerIndex].sprite->width() * 1.4)),
                         (team1[playerIndex].sprite->yPos() +
                          team1[playerIndex].sprite->height()) };
      newTopLeft     = { newXPos, team1[playerIndex].sprite->yPos() };
      newBottomLeft  = { newXPos,
                        (team1[playerIndex].sprite->yPos() + team1[playerIndex].sprite->height()) };

      std::array<ASGE::Point2D, 2> newPoints;

      if (moveUnitsX > 0)
      {
        newPoints[0] = newTopRight;
        newPoints[1] = newBottomRight;
      }
      if (moveUnitsX < 0)
      {
        newPoints[0] = newTopLeft;
        newPoints[1] = newBottomLeft;
      }

      // iterate through collidables list
      // inhibit player's horizontal movement if they collide with anything solid
      for (const auto& i : collidables)
      {
        if (directionalCollisionCheck(i.get(), newPoints))
        {
          std::cout << "collide\n";
          team1[playerIndex].setCanMoveInX(false);
        }
      }

      if (!team1[playerIndex].getCanMoveInX())
      {
        a = 0;
      }

      team1[playerIndex].sprite->xPos(team1[playerIndex].sprite->xPos() + a);
      team1[playerIndex].movementRangeSprite->xPos(
        team1[playerIndex].movementRangeSprite->xPos() + a);
      team1[playerIndex].shootingRangeSprite->xPos(
        team1[playerIndex].shootingRangeSprite->xPos() + a);

      team1[playerIndex].setCanMoveInY(true);

      float newYPos = team1[playerIndex].sprite->yPos() + b;

      newTopRight    = { static_cast<float>(
                        team1[playerIndex].sprite->xPos() +
                        (team1[playerIndex].sprite->width() * 1.4)),
                      newYPos };
      newBottomRight = { static_cast<float>(
                           team1[playerIndex].sprite->xPos() +
                           (team1[playerIndex].sprite->width() * 1.4)),
                         (newYPos + team1[playerIndex].sprite->height()) };
      newTopLeft     = { team1[playerIndex].sprite->xPos(), newYPos };
      newBottomLeft  = { team1[playerIndex].sprite->xPos(),
                        (newYPos + team1[playerIndex].sprite->height()) };

      if (moveUnitsY < 0)
      {
        newPoints[0] = newTopRight;
        newPoints[1] = newTopLeft;
      }
      if (moveUnitsY > 0)
      {
        newPoints[0] = newBottomRight;
        newPoints[1] = newBottomLeft;
      }

      // iterate through collidables list
      // inhibit player's horizontal movement if they collide with anything solid
      for (const auto& i : collidables)
      {
        if (directionalCollisionCheck(i.get(), newPoints))
        {
          team1[playerIndex].setCanMoveInY(false);
        }
      }

      if (!team1[playerIndex].getCanMoveInY())
      {
        b = 0;
      }

      team1[playerIndex].sprite->yPos(team1[playerIndex].sprite->yPos() + b);
      team1[playerIndex].movementRangeSprite->yPos(
        team1[playerIndex].movementRangeSprite->yPos() + b);
      team1[playerIndex].shootingRangeSprite->yPos(
        team1[playerIndex].shootingRangeSprite->yPos() + b);

      lerpIndex++;
    }
    else
    {
      Net.send(
        NetSocket,
        std::string("1~") + std::to_string(playerIndex) + "~" +
          std::to_string(team1[playerIndex].sprite->xPos()) + "~" +
          std::to_string(team1[playerIndex].sprite->yPos()) + "~00");

      lerpIndex = 0;
      team1[playerIndex].setIsMoving(false);
      team1[playerIndex].setInShootingMode(true);
      Net.send(
        NetSocket,
        std::string("1~") + std::to_string(playerIndex) + "~" +
          std::to_string(team1[playerIndex].sprite->xPos()) + "~" +
          std::to_string(team1[playerIndex].sprite->yPos()) + "~00");
    }
  }

  else
  {
    float a = (moveUnitsX / lerpFactor);
    float b = (moveUnitsY / lerpFactor);

    if (lerpIndex <= lerpFactor)
    {
      team2[playerIndex].setCanMoveInX(true);

      float newXPos = team2[playerIndex].sprite->xPos() + a;

      ASGE::Point2D newTopRight;
      ASGE::Point2D newBottomRight;
      ASGE::Point2D newTopLeft;
      ASGE::Point2D newBottomLeft;

      newTopRight    = { static_cast<float>(newXPos + (team2[playerIndex].sprite->width() * 1.4)),
                      team2[playerIndex].sprite->yPos() };
      newBottomRight = { static_cast<float>(newXPos + (team2[playerIndex].sprite->width() * 1.4)),
                         (team2[playerIndex].sprite->yPos() +
                          team2[playerIndex].sprite->height()) };
      newTopLeft     = { newXPos, team2[playerIndex].sprite->yPos() };
      newBottomLeft  = { newXPos,
                        (team2[playerIndex].sprite->yPos() + team2[playerIndex].sprite->height()) };

      std::array<ASGE::Point2D, 2> newPoints;

      if (moveUnitsX > 0)
      {
        newPoints[0] = newTopRight;
        newPoints[1] = newBottomRight;
      }
      if (moveUnitsX < 0)
      {
        newPoints[0] = newTopLeft;
        newPoints[1] = newBottomLeft;
      }

      // iterate through collidables list
      // inhibit player's horizontal movement if they collide with anything solid
      for (const auto& i : collidables)
      {
        if (directionalCollisionCheck(i.get(), newPoints))
        {
          std::cout << "collide\n";
          team2[playerIndex].setCanMoveInX(false);
        }
      }

      if (!team2[playerIndex].getCanMoveInX())
      {
        a = 0;
      }

      team2[playerIndex].sprite->xPos(team2[playerIndex].sprite->xPos() + a);
      team2[playerIndex].movementRangeSprite->xPos(
        team2[playerIndex].movementRangeSprite->xPos() + a);
      team2[playerIndex].shootingRangeSprite->xPos(
        team2[playerIndex].shootingRangeSprite->xPos() + a);

      team2[playerIndex].setCanMoveInY(true);

      float newYPos = team2[playerIndex].sprite->yPos() + b;

      newTopRight    = { static_cast<float>(
                        team2[playerIndex].sprite->xPos() +
                        (team2[playerIndex].sprite->width() * 1.4)),
                      newYPos };
      newBottomRight = { static_cast<float>(
                           team2[playerIndex].sprite->xPos() +
                           (team2[playerIndex].sprite->width() * 1.4)),
                         (newYPos + team2[playerIndex].sprite->height()) };
      newTopLeft     = { team2[playerIndex].sprite->xPos(), newYPos };
      newBottomLeft  = { team2[playerIndex].sprite->xPos(),
                        (newYPos + team2[playerIndex].sprite->height()) };

      if (moveUnitsY < 0)
      {
        newPoints[0] = newTopRight;
        newPoints[1] = newTopLeft;
      }
      if (moveUnitsY > 0)
      {
        newPoints[0] = newBottomRight;
        newPoints[1] = newBottomLeft;
      }

      // iterate through collidables list
      // inhibit player's horizontal movement if they collide with anything solid
      for (const auto& i : collidables)
      {
        if (directionalCollisionCheck(i.get(), newPoints))
        {
          team2[playerIndex].setCanMoveInY(false);
        }
      }

      if (!team2[playerIndex].getCanMoveInY())
      {
        b = 0;
      }

      team2[playerIndex].sprite->yPos(team2[playerIndex].sprite->yPos() + b);
      team2[playerIndex].movementRangeSprite->yPos(
        team2[playerIndex].movementRangeSprite->yPos() + b);
      team2[playerIndex].shootingRangeSprite->yPos(
        team2[playerIndex].shootingRangeSprite->yPos() + b);

      lerpIndex++;
    }
    else
    {
      lerpIndex = 0;
      team2[playerIndex].setIsMoving(false);
      team2[playerIndex].setInShootingMode(true);
      Net.send(
        NetSocket,
        std::string("1~") + std::to_string(playerIndex) + "~" +
          std::to_string(team2[playerIndex].sprite->xPos()) + "~" +
          std::to_string(team2[playerIndex].sprite->yPos()) + "~00");
    }
  }
}

// returns true if either collPoint Point2D is inside the target sprite
// used to calculate collision in a given direction, with two forward-facing collPoints
bool ASGENetGame::directionalCollisionCheck(
  const ASGE::Sprite* target, std::array<ASGE::Point2D, 2> collPoints)
{
  // iterate through points array
  for (unsigned int i = 0; i < 2; ++i)
  {
    if (collPoints[i].x > target->xPos())
    {
      if (collPoints[i].x < target->xPos() + target->width())
      {
        if (collPoints[i].y > target->yPos())
        {
          if (collPoints[i].y < target->yPos() + target->height())
          {
            return true;
          }
        }
      }
    }
  }
  return false;
}

void ASGENetGame::shoot(unsigned int playerIndex, bool team)
{
  Net.send(
    NetSocket,
    std::string("2~") + std::to_string(00) + "~" + std::to_string(00) + "~" + std::to_string(00) +
      "~00");

  if (team)
  {
    projectileIndex += 1;
    if (projectileIndex >= 9)
    {
      projectileIndex = 0;
    }

    projectiles[projectileIndex].setIsActive(true);

    projectiles[projectileIndex].sprite->xPos(team1[playerIndex].sprite->xPos());
    projectiles[projectileIndex].sprite->yPos(team1[playerIndex].sprite->yPos());

    team1[playerIndex].setInShootingMode(false);
  }

  else
  {
    projectileIndex += 1;
    if (projectileIndex >= 9)
    {
      projectileIndex = 0;
    }

    projectiles[projectileIndex].setIsActive(true);

    projectiles[projectileIndex].sprite->xPos(team2[playerIndex].sprite->xPos());
    projectiles[projectileIndex].sprite->yPos(team2[playerIndex].sprite->yPos());

    team2[playerIndex].setInShootingMode(false);
  }
  isMyTurn = false;
}

bool ASGENetGame::initProjectiles()
{
  for (unsigned int k = 0; k <= 8; ++k)
  {
    projectiles[k].sprite = renderer->createUniqueSprite();

    if (!projectiles[k].sprite->loadTexture("/data/img/hazards/cannonball.png"))
    {
      // report error message if texture load fails
      std::cout << "error - failed to load cannonball\n";
      return false;
    }

    projectiles[k].sprite->width(16);
    projectiles[k].sprite->height(16);
    projectiles[k].sprite->scale(static_cast<float>(1.4));
    projectiles[k].sprite->setMagFilter(ASGE::GameSettings::MagFilter::NEAREST);

    projectiles[k].setIsActive(false);
  }

  return true;
}

void ASGENetGame::moveProjectile(float moveUnitsX, float moveUnitsY)
{
  float a = (moveUnitsX / projectileLerpFactor);
  float b = (moveUnitsY / projectileLerpFactor);

  if (projectileLerpIndex <= projectileLerpFactor)
  {
    float newXPos = projectiles[projectileIndex].sprite->xPos() + a;

    projectiles[projectileIndex].sprite->xPos(projectiles[projectileIndex].sprite->xPos() + a);

    ASGE::Point2D newTopRight;
    ASGE::Point2D newBottomRight;
    ASGE::Point2D newTopLeft;
    ASGE::Point2D newBottomLeft;

    newTopRight    = { static_cast<float>(
                      newXPos + (projectiles[projectileIndex].sprite->width() * 1.4)),
                    projectiles[projectileIndex].sprite->yPos() };
    newBottomRight = { static_cast<float>(
                         newXPos + (projectiles[projectileIndex].sprite->width() * 1.4)),
                       (static_cast<float>(
                         projectiles[projectileIndex].sprite->yPos() +
                         projectiles[projectileIndex].sprite->height() * 1.4)) };
    newTopLeft     = { newXPos, projectiles[projectileIndex].sprite->yPos() };
    newBottomLeft  = { newXPos,
                      (static_cast<float>(
                        projectiles[projectileIndex].sprite->yPos() +
                        projectiles[projectileIndex].sprite->height() * 1.4)) };

    std::array<ASGE::Point2D, 2> newPoints;

    if (moveUnitsX > 0)
    {
      newPoints[0] = newTopRight;
      newPoints[1] = newBottomRight;
    }
    if (moveUnitsX < 0)
    {
      newPoints[0] = newTopLeft;
      newPoints[1] = newBottomLeft;
    }

    // iterate through collidables list
    // inhibit player's horizontal movement if they collide with anything solid
    for (const auto& i : collidables)
    {
      if (directionalCollisionCheck(i.get(), newPoints))
      {
        std::cout << "projectile collide\n";
        projectileLerpIndex = 0;
        projectiles[projectileIndex].setIsActive(false);
      }
    }

    for (unsigned int i = 0; i <= 3; ++i)
    {
      if (isBlueTeam)
      {
        for (unsigned int n = 0; n < 2; ++n)
        {
          if (team2[i].getIsAlive())
          {
            if (newPoints[n].x > team2[i].sprite->xPos())
            {
              if (newPoints[n].x < team2[i].sprite->xPos() + (team2[i].sprite->width()) * 1.4)
              {
                if (newPoints[n].y > team2[i].sprite->yPos())
                {
                  if (newPoints[n].y < team2[i].sprite->yPos() + (team2[i].sprite->height()) * 1.4)
                  {
                    //                std::cout << "projectile collide with player\n";
                    projectileLerpIndex = 0;
                    projectiles[projectileIndex].setIsActive(false);
                    team2[i].applyDamage(1);
                    std::cout << team2[i].getHealth() << "\n";
                  }
                }
              }
            }
          }
        }
      }

      else
      {
        for (unsigned int n = 0; n < 2; ++n)
        {
          if (team1[i].getIsAlive())
          {
            if (newPoints[n].x > team1[i].sprite->xPos())
            {
              if (newPoints[n].x < team1[i].sprite->xPos() + (team1[i].sprite->width()) * 1.4)
              {
                if (newPoints[n].y > team1[i].sprite->yPos())
                {
                  if (newPoints[n].y < team1[i].sprite->yPos() + (team1[i].sprite->height()) * 1.4)
                  {
                    //                std::cout << "projectile collide with player\n";
                    projectileLerpIndex = 0;
                    projectiles[projectileIndex].setIsActive(false);
                    team1[i].applyDamage(1);
                    std::cout << team1[i].getHealth() << "\n";
                  }
                }
              }
            }
          }
        }
      }
    }

    projectiles[projectileIndex].sprite->yPos(projectiles[projectileIndex].sprite->yPos() + b);
    projectileLerpIndex++;
  }

  else
  {
    projectileLerpIndex = 0;
    projectiles[projectileIndex].setIsActive(false);
  }
}

void ASGENetGame::clientlisten(kissnet::tcp_socket& socket)
{
  while (Net.running && Net.connected)
  {
    // same size as the server buffer
    kissnet::buffer<4096> static_buffer;
    while (Net.connected)
    {
      // structured bindings in play again
      if (auto [size, valid] = socket.recv(static_buffer); valid)
      {
        if (valid.value == kissnet::socket_status::cleanly_disconnected)
        {
          Net.connected = false;
          std::cout << "clean disconnection" << std::endl;
          socket.close();
          break;
        }

        // treat all data as string and append null terminator
        if (size < static_buffer.size())
          static_buffer[size] = std::byte{ 0 };
        std::string s      = std::string(reinterpret_cast<char*>(static_buffer.data()));
        int action         = 0;
        unsigned int index = 0;
        float x            = 0;
        float y            = 0;

        std::string temp;
        int cnt = 0;
        for (char& c : s)
        {
          if (c == '~')
          {
            cnt++;
            switch (cnt)
            {
              case 1:
                action = std::stoi(temp);
                break;
              case 2:
                index = static_cast<unsigned int>(std::stoi(temp));
                break;
              case 3:
                x = std::stof(temp);
                break;
              case 4:
                y = std::stof(temp);
                break;
              case 5:
                cnt = 0;
                break;
            }
            temp = "";
          }
          else
          {
            temp = temp + c;
            // std::cout << temp << std::endl;
          }
        }

        switch (action)
        {
          case 1:
            if (isBlueTeam)
            {
              team2[index].sprite->xPos(x);
              team2[index].sprite->yPos(y);
            }
            else
            {
              team1[index].sprite->xPos(x);
              team1[index].sprite->yPos(y);
            }
            break;
          case 2:
            isMyTurn = true;
            break;
          case 3:
            if (x == 1.0f)
            {
              team1[index].setIsAlive(false);
            }
            else
            {
              team2[index].setIsAlive(false);
            }
            break;
          case 4:
            if (index == 1)
            {
              Net.send(
                NetSocket,
                std::string("4~") + std::to_string(2) + "~" + std::to_string(00) + "~" +
                  std::to_string(00) + "~00");
              inLobby    = false;
              inGame     = true;
              isBlueTeam = true;
              isMyTurn   = true;
            }
            if (index == 2)
            {
              inLobby    = false;
              inGame     = true;
              isBlueTeam = false;
              isMyTurn   = false;
            }
        }
      }
      else
      {
        Net.connected = false;
        std::cout << "disconnected" << std::endl;
        socket.close();
      }
    }
  }
}

// initialise fonts
void ASGENetGame::initFont()
{
  // load font for the lives counter, assign attributes
  if (auto idx = renderer->loadFont("/data/fonts/EdgeOfTheGalaxyItalic-ZVJB3.otf", 48); idx > 0)
  {
    fontIdx = idx;
    turnIndicator.setFont(renderer->getFont(fontIdx))
      .setString("YOUR TURN!!")
      .setPosition({ 10, 10 })
      .setColour(ASGE::COLOURS::RED);
    turnIndicator.setPositionX(20);
    turnIndicator.setPositionY(65);
  }
}
