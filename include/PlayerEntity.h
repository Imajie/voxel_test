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
#include "Ogre.h"
#include <string>

class PlayerEntity : public GameObject
{
	public:
		PlayerEntity( uint32_t hostAddr, std::string playerName );
		virtual ~PlayerEntity();

		virtual Packet* serialize() const;
		virtual bool unserialize( Packet &packet );

		const std::string& getName() const
		{
			return playerName;
		}

		void setName( std::string& newName )
		{
			playerName = newName;
		}

		const uint32_t getId() const
		{
			return hostAddr;
		}

		void setLocation(Ogre::Vector3 newLocation)
		{
			location = newLocation;
		}

		Ogre::Vector3 getLocation()
		{
			return location;
		}

	private:
		std::string playerName;
		uint32_t hostAddr;

	protected:
		// for the player's location
		Ogre::Vector3 location;
};

#endif
