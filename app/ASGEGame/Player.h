//
// Created by Joseph Khan on 01/02/2022.
//

#ifndef ASGENETGAME_PLAYER_H
#define ASGENETGAME_PLAYER_H
#include <ASGEGameLib/GComponent.hpp>
#include <Engine/OGLGame.hpp>
#include <Engine/Sprite.hpp>

class Player
{
 public:
  Player();
  ~Player();
  std::unique_ptr<ASGE::Sprite> sprite              = nullptr;
  std::unique_ptr<ASGE::Sprite> movementRangeSprite = nullptr;
  std::unique_ptr<ASGE::Sprite> shootingRangeSprite = nullptr;
  int getHealth();
  void setHealth(int newHealth);
  void applyDamage(int damage);
  bool getIsSelected();
  void select();
  void deselect();
  bool getRenderMovementSprite();
  void setRenderMovementSprite(bool newRenderMovementSprite);
  bool getIsMoving();
  void setIsMoving(bool newIsMoving);
  bool getCanMoveInX();
  void setCanMoveInX(bool newCanMoveInX);
  bool getCanMoveInY();
  void setCanMoveInY(bool newCanMoveInY);
  bool getInShootingMode();
  void setInShootingMode(bool newInShootingMode);
  bool getRenderShootingSprite();
  void setRenderShootingSprite(bool newRenderShootingSprite);
  bool getIsAlive();
  void setIsAlive(bool newIsAlive);
  bool serverKnows = false;

 private:
  int health                = 2;
  bool isSelected           = false;
  bool renderMovementSprite = false;
  bool isMoving             = false;
  bool canMoveInX           = true;
  bool canMoveInY           = true;
  bool inShootingMode       = false;
  bool renderShootingSprite = false;
  bool isAlive              = true;
};

#endif // ASGENETGAME_PLAYER_H
