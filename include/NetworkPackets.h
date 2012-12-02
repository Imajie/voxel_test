/*
 * File:	NetworkPackets.h
 * Author:	James Letendre
 *
 * Defines for network packets
 */
#ifndef NETWORK_PACKETS_H
#define NETWORK_PACKETS_H

#include <list>
#include <cstdint>
#include <enet/enet.h>
#include <iostream>

enum PacketType {
	// connection sequence
	CONNECTION_REQUEST_SYNC,	// Request sync of entities from the server
	CONNECTION_SYNC_FINISHED,	// Finished sending entities for sync
	CONNECTION_CLIENT_ID,		// Our ID from the server

	// player messages
	PLAYER_SET_USERNAME,		// set the username for this player
	PLAYER_CONNECT,				// player joined server
	PLAYER_DISCONNECT,			// player left server
	PLAYER_MOVE,				// player moved
	PLAYER_SYNC,				// sync player from server

	// terrain messages
	TERRAIN_REQUEST,			// load request
	TERRAIN_RESPONSE,			// load response
	TERRAIN_UPDATE,				// modification to voxel(s)

	// End of packets
	PACKET_TYPE_SENTINEL		// MUST BE LAST
};

class Packet {
	public:
		PacketType type;

		void dumpDebug()
		{
			std::cerr << (int)type << " ";
			for( auto c : data )
			{
				std::cerr << (int)c << " ";
			}
			std::cerr << std::endl << std::endl;
		}
		const char* serialize();
		void unserialize( const unsigned char* data, size_t size );

		// send the packet
		void send( ENetPeer *client, uint32_t flags );
		void broadcast( ENetHost *server, uint32_t flags );

		size_t getSize() const { return data.size(); }

		template<typename T>
			void push( T x )
			{
				if( sizeof(T) == 1 )
				{
					data.push_back(x);
				}
				else if( sizeof(T) == 2 )
				{
					x = htons(x);

					data.push_back(((uint8_t*)&x)[0]);
					data.push_back(((uint8_t*)&x)[1]);
				}
				else if( sizeof(T) == 4 )
				{
					x = htonl(x);

					data.push_back(((uint8_t*)&x)[0]);
					data.push_back(((uint8_t*)&x)[1]);
					data.push_back(((uint8_t*)&x)[2]);
					data.push_back(((uint8_t*)&x)[3]);
				}
			}

		template<typename T>
			T pop()
			{
				T val;

				if( sizeof(val) == 1 )
				{
					val = data.front(); data.pop_front();
				}
				else if( sizeof(val) == 2 )
				{
					((uint8_t*)&val)[0] = data.front(); data.pop_front();
					((uint8_t*)&val)[1] = data.front(); data.pop_front();

					val = ntohs(val);
				}
				else if( sizeof(val) == 4 )
				{
					((uint8_t*)&val)[0] = data.front(); data.pop_front();
					((uint8_t*)&val)[1] = data.front(); data.pop_front();
					((uint8_t*)&val)[2] = data.front(); data.pop_front();
					((uint8_t*)&val)[3] = data.front(); data.pop_front();

					val = ntohl(val);
				}

				return val;
			}

	private:
		std::list<char> data;
};


#endif
