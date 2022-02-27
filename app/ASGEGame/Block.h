//
// Created by Joseph on 09/02/2022.
//

#ifndef ASGENETGAME_BLOCK_H
#define ASGENETGAME_BLOCK_H
#include <ASGEGameLib/GComponent.hpp>
#include <Engine/OGLGame.hpp>
#include <Engine/Sprite.hpp>

class Block
{
 public:
  Block();
  ~Block();
  bool getIsActive();
  void setIsActive(bool newIsActive);
  std::unique_ptr<ASGE::Sprite> sprite = nullptr;

 private:
  bool isActive = true;
};

#endif // ASGENETGAME_BLOCK_H
