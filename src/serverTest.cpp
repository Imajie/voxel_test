/*
 * Enet library test server
 */

#include <enet/enet.h>
#include <iostream>
#include <cstdint>

using namespace std;

int main( int argc, char **argv )
{
	// initialize ENet
	if( enet_initialize() != 0 )
	{
		cerr << "Error initializing ENet" << endl;
		exit(EXIT_FAILURE);
	}

	// add hook to run on exit
	atexit(enet_deinitialize);
	
	// create simple server
	ENetAddress address;
	ENetHost *server;

	// create server on local host on port 1234
	address.host = ENET_HOST_ANY;
	address.port = 4321;

	cout << "Creating server" << endl;
	// create server with max 32 clients, 2 channels and now bandwidth limits
	server = enet_host_create( &address, 32, 2, 0, 0 );

	if( !server )
	{
		cerr << "Error creating ENet server" << endl;
		exit(EXIT_FAILURE);
	}

	ENetEvent event;
	ENetPacket *packet;
	int value;

	// wait up to 1000 ms for a request
	while( enet_host_service( server, &event, 1000 ) >= 0 )
	{
		switch( event.type )
		{
			case ENET_EVENT_TYPE_CONNECT:
				// new connection request
				cout << "New connection request from " << event.peer->address.host << ":" << event.peer->address.port << endl;

				event.peer->data = (void*)"Client info goes here";
				break;

			case ENET_EVENT_TYPE_RECEIVE:
				// new data from a client
				value = *(int*)event.packet->data;
				cout << "Packet received val = " << value << endl;

				// increment and resend
				value++;
				packet = enet_packet_create( &value, sizeof(value), ENET_PACKET_FLAG_RELIABLE );

				enet_peer_send( event.peer, 0, packet );
				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				// client disconnect
				cout << "Client "  << event.peer->data << " disconnect" << endl;

				break;
			case ENET_EVENT_TYPE_NONE:
				// nothing happened
				break;
		}
	}
	cout << "Quitting" << endl;

	enet_host_destroy(server);

	return 0;
}

