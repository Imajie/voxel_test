/*
 * File:	NetworkPackets.h
 * Author:	James Letendre
 *
 * Defines for network packets
 */
#ifndef NETWORK_PACKETS_H
#define NETWORK_PACKETS_H

#include <vector>
#include <cstdint>
#include <enet/enet.h>

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
		std::vector<char> data;

		const char* serialize();
		void unserialize( const unsigned char* data, size_t size );

		// send the packet
		void send( ENetPeer *client, uint32_t flags );
		void broadcast( ENetHost *server, uint32_t flags );
};


#endif
