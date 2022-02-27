//
// Created by Joseph on 07/02/2022.
//

#include "Projectile.h"

Projectile::Projectile() {}

Projectile::~Projectile() {}

bool Projectile::getIsActive()
{
  return isActive;
}

void Projectile::setIsActive(bool newIsActive)
{
  isActive = newIsActive;
}
