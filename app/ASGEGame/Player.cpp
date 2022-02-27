//
// Created by Joseph Khan on 01/02/2022.
//

#include "Player.h"

Player::Player() {}

Player::~Player() {}

int Player::getHealth()
{
  return health;
}

void Player::setHealth(int newHealth)
{
  health = newHealth;
}

void Player::applyDamage(int damage)
{
  health -= damage;
}

bool Player::getIsSelected()
{
  return isSelected;
}

void Player::select()
{
  isSelected = true;
}

void Player::deselect()
{
  isSelected = false;
}

bool Player::getRenderMovementSprite()
{
  return renderMovementSprite;
}

void Player::setRenderMovementSprite(bool newRenderMovementSprite)
{
  renderMovementSprite = newRenderMovementSprite;
}

bool Player::getIsMoving()
{
  return isMoving;
}

void Player::setIsMoving(bool newIsMoving)
{
  isMoving = newIsMoving;
}

bool Player::getCanMoveInX()
{
  return canMoveInX;
}

void Player::setCanMoveInX(bool newCanMoveInX)
{
  canMoveInX = newCanMoveInX;
}

bool Player::getCanMoveInY()
{
  return canMoveInY;
}

void Player::setCanMoveInY(bool newCanMoveInY)
{
  canMoveInY = newCanMoveInY;
}

bool Player::getInShootingMode()
{
  return inShootingMode;
}

void Player::setInShootingMode(bool newInShootingMode)
{
  inShootingMode = newInShootingMode;
}

bool Player::getRenderShootingSprite()
{
  return renderShootingSprite;
}

void Player::setRenderShootingSprite(bool newRenderShootingSprite)
{
  renderShootingSprite = newRenderShootingSprite;
}

bool Player::getIsAlive()
{
  return isAlive;
}

void Player::setIsAlive(bool newIsAlive)
{
  isAlive = newIsAlive;
}