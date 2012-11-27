/*
 * File:	PlayerEntity.h
 * Author:	James Letendre
 *
 * Represents a player
 */
#ifndef PLAYER_ENTITY_H
#define PLAYER_ENTITY_H

#include "GameObject.h"
#include "NetworkPackets.h"
#include <string>

class PlayerEntity : public GameObject
{
	public:
		PlayerEntity( std::string playerName );
		virtual ~PlayerEntity();

		virtual Packet* serialize() const;
		virtual bool unserialize( Packet &packet );

		const std::string& getName() const
		{
			return playerName;
		}

	private:
		std::string playerName;
};

#endif
