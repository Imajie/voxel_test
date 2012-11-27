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
	// player messages
	PLAYER_CONNECT_PACKET,		// player joined server
	PLAYER_DISCONNECT_PACKET,	// player left server
	PLAYER_MOVE_PACKET,			// player moved

	// terrain messages
	TERRAIN_REQUEST_PACKET,		// load request
	TERRAIN_RESPONSE_PACKET,	// load response
	TERRAIN_UPDATE_PACKET,		// modification to voxel(s)

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
