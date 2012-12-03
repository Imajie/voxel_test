/*
 * File:	NetworkPackets.h
 * Author:	James Letendre
 *
 * Defines for network packets
 */
#ifndef NETWORK_PACKETS_H
#define NETWORK_PACKETS_H

#include <list>
#include <vector>
#include <cstdint>
#include <enet/enet.h>
#include <iostream>

enum PacketType {
	PACKET_NONE, 				// Default packet type, not valid to transmit

	// connection sequence
	CONNECTION_REQUEST_SYNC,	// Request sync of entities from the server
	CONNECTION_SYNC_FINISHED,	// Finished sending entities for sync
	CONNECTION_CLIENT_ID,		// Our ID from the server

	// player messages
	PLAYER_SET_USERNAME,		// set the username for this player
	PLAYER_CONNECT,				// player joined server
	PLAYER_DISCONNECT,			// player left server
	PLAYER_MOVE,				// player moved
	PLAYER_DIRECTION,			// player changed direction
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

		void clear() { data.clear(); type = PACKET_NONE; }
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
				else if( sizeof(T) == 8 )
				{
					uint32_t val_a, val_b;

					val_a = (uint32_t)((uint64_t)x>>32);
					val_b = (uint32_t)(x & 0xFFFFFFFF);

					val_b = htonl(val_b);
					val_a = htonl(val_a);

					data.push_back(((uint8_t*)&val_a)[0]);
					data.push_back(((uint8_t*)&val_a)[1]);
					data.push_back(((uint8_t*)&val_a)[2]);
					data.push_back(((uint8_t*)&val_a)[3]);

					data.push_back(((uint8_t*)&val_b)[0]);
					data.push_back(((uint8_t*)&val_b)[1]);
					data.push_back(((uint8_t*)&val_b)[2]);
					data.push_back(((uint8_t*)&val_b)[3]);
				}
			}

		/*
		   void push( float x )
		   {
		   uint32_t val = *(uint32_t*)&x;
		   push<uint32_t>(val);
		   }
		   */

		void push( double x )
		{
			union{ double d; uint64_t i; } diu;

			diu.d = x;
			push<uint64_t>(diu.i);
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
				else if( sizeof(val) == 8 )
				{
					uint32_t val_a, val_b;

					((uint8_t*)&val_a)[0] = data.front(); data.pop_front();
					((uint8_t*)&val_a)[1] = data.front(); data.pop_front();
					((uint8_t*)&val_a)[2] = data.front(); data.pop_front();
					((uint8_t*)&val_a)[3] = data.front(); data.pop_front();

					((uint8_t*)&val_b)[0] = data.front(); data.pop_front();
					((uint8_t*)&val_b)[1] = data.front(); data.pop_front();
					((uint8_t*)&val_b)[2] = data.front(); data.pop_front();
					((uint8_t*)&val_b)[3] = data.front(); data.pop_front();

					val_a = ntohl(val_a);
					val_b = ntohl(val_b);

					val = ( ((uint64_t)val_a) << 32) | val_b;
				}

				return val;
			}

		/*
		float pop()
		{
			uint32_t val = pop<uint32_t>();

			return *(float*)(&val);
		}
		*/

		double pop()
		{
			union{ double d; uint64_t i; } diu;

			diu.i = pop<uint64_t>();

			return diu.d;
		}

		template<typename V>
			void push_vector( typename V::const_iterator begin, typename V::const_iterator end )
			{
				for( auto it = begin; it != end; it++ )
				{
					push(*it);
				}
			}

	private:
		std::list<char> data;
};


#endif
