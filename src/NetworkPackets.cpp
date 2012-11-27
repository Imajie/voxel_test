/*
 * File:	NetworkPackets.cpp
 * Author:	James Letendre
 *
 * Defines for network packets
 */

#include "NetworkPackets.h"

const char* Packet::serialize()
{
	char *buffer = new char[data.size()+sizeof(size_t)+sizeof(PacketType)];

	((PacketType*)buffer)[0] = type;
	((size_t*)(buffer+sizeof(PacketType)))[0] = data.size();

	size_t buffer_offset = sizeof(PacketType) + sizeof(size_t);
	for( char c : data )
	{
		buffer[buffer_offset++] = c;
	}

	return buffer;
}

void Packet::unserialize( const char* packet )
{
	data.clear();

	type = ((PacketType*)packet)[0];

	size_t size = ((size_t*)(packet+sizeof(PacketType)))[0];

	const char* ptr = packet + sizeof(PacketType) + sizeof(size_t);
	for( size_t i = 0; i < size; i++ )
	{
		data.push_back(*ptr);
		ptr++;
	}
}

/* 
 * send a packet to the specified host
 *
 * send packet to the client
 */
void Packet::send( ENetPeer *client, uint32_t packet_flags )
{
	const char *buffer = serialize();
	size_t buffer_size = data.size() + sizeof( size_t ) + sizeof( PacketType );

	ENetPacket *packet = enet_packet_create( buffer, buffer_size, packet_flags );

	enet_peer_send( client, 0, packet );

	delete [] buffer;
}

/* 
 * send a packet to all clients
 */
void Packet::broadcast( ENetHost *server, uint32_t packet_flags )
{
	const char *buffer = serialize();
	size_t buffer_size = data.size() + sizeof( size_t ) + sizeof( PacketType );

	ENetPacket *packet = enet_packet_create( buffer, buffer_size, packet_flags );

	enet_host_broadcast( server, 0, packet );

	delete [] buffer;
}

