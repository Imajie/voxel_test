/*
 * File:	PlayerEntity.cpp
 * Author:	James Letendre
 *
 * Represents a player
 */
#include "PlayerEntity.h"

#include <iostream>

PlayerEntity::PlayerEntity( uint32_t hostAddr, std::string playerName ) : 
	playerName(playerName), hostAddr( hostAddr )
{
}

PlayerEntity::~PlayerEntity()
{
}

Packet* PlayerEntity::serialize() const
{
	Packet *packet = new Packet;

	// send as ID, nameLen, name
	packet->push<uint32_t>( hostAddr );
	packet->push<uint8_t>( playerName.length() );

	for( size_t i = 0; i < playerName.size(); i++ )
	{
		packet->push<char>( playerName[i] );
	}
	return packet;
}

bool PlayerEntity::unserialize( Packet &packet )
{
	if( packet.getSize() > sizeof( uint32_t ) + 1 )
	{
		uint32_t newId = packet.pop<uint32_t>();
		uint8_t nameLen = packet.pop<uint8_t>();

		if( packet.getSize() == nameLen )
		{
			hostAddr = newId;
			playerName = "";

			for( int i = 0; i < nameLen; i++ )
			{
				playerName.push_back( packet.pop<char>() );
			}
			return true;
		}
		std::cout << "PlayerEntity.unserialize: expected " << nameLen << " bytes for name, got " << packet.getSize() << std::endl;
		return false;
	}
	std::cout << "PlayerEntity.unserialize: Packet too small" << std::endl;
	return false;
}
