/*
 * File:	PlayerEntity.cpp
 * Author:	James Letendre
 *
 * Represents a player
 */
#include "PlayerEntity.h"

PlayerEntity::PlayerEntity( std::string playerName ) : 
	playerName(playerName)
{
}

PlayerEntity::~PlayerEntity()
{
}

Packet* PlayerEntity::serialize() const
{

	return NULL;
}

bool PlayerEntity::unserialize( Packet &packet )
{

	return false;
}
