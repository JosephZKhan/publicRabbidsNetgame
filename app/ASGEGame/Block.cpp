//
// Created by Joseph on 09/02/2022.
//

#include "Block.h"

Block::Block() {}

Block::~Block() {}

bool Block::getIsActive()
{
  return isActive;
}

void Block::setIsActive(bool newIsActive)
{
  isActive = newIsActive;
}
