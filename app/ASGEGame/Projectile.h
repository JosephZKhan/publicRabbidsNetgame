//
// Created by Joseph on 07/02/2022.
//

#ifndef ASGENETGAME_PROJECTILE_H
#define ASGENETGAME_PROJECTILE_H
#include <ASGEGameLib/GComponent.hpp>
#include <Engine/OGLGame.hpp>
#include <Engine/Sprite.hpp>

class Projectile
{
 public:
  Projectile();
  ~Projectile();
  std::unique_ptr<ASGE::Sprite> sprite = nullptr;
  bool getIsActive();
  void setIsActive(bool newIsActive);

 private:
  bool isActive;
};

#endif // ASGENETGAME_PROJECTILE_H
