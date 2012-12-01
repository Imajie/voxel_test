/*
 * File:	PlayerEntity.cpp
 * Author:	James Letendre
 *
 * Represents a player
 */
#include "PlayerEntity.h"

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
	packet->data = std::vector<char>(sizeof(uint32_t)+sizeof(uint8_t)+playerName.length());

	uint8_t *data_ptr = (uint8_t*)(&packet->data[0]);

	((uint32_t*)data_ptr)[0] = htonl(hostAddr);
	data_ptr[sizeof(uint32_t)] = (uint8_t)playerName.length();

	size_t offset = sizeof(uint32_t) + 1;
	for( size_t i = 0; i < playerName.size(); i++ )
	{
		data_ptr[offset++] = playerName[i];
	}
	return packet;
}

bool PlayerEntity::unserialize( Packet &packet )
{
	if( packet.data.size() > sizeof( uint32_t ) + 1 )
	{
		uint8_t *data_ptr = (uint8_t*)(&packet.data[0]);

		uint32_t newId = ntohl( ((uint32_t*)data_ptr)[0] );
		uint8_t nameLen = data_ptr[sizeof(uint32_t)];

		if( packet.data.size() == sizeof(uint32_t)+1+nameLen )
		{
			hostAddr = newId;
			playerName = std::string( packet.data.begin()+sizeof(uint32_t)+1, packet.data.end() );
			return true;
		}
		return false;
	}
	return false;
}
