/*
 * ENet library test client
 */

#include <enet/enet.h>

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <string>
#include <signal.h>

using namespace std;

bool quit = false;

void kill_sig( int code )
{
	quit = true;
}

int main( int argc, char ** argv )
{
	signal( SIGKILL, kill_sig );
	signal( SIGTERM, kill_sig );
	signal( SIGINT,  kill_sig );

	// initialize ENet
	if( enet_initialize() != 0 )
	{
		cerr << "Error initializing ENet" << endl;
		exit(EXIT_FAILURE);
	}

	// add hook to run on exit
	atexit(enet_deinitialize);

	ENetHost *client;

	// create an ENet client, limit 1 connection (to server), 2 channels, no bandwidth limits
	client = enet_host_create( NULL, 1, 2, 0, 0 );

	if( !client )
	{
		cerr << "Error creating ENet client" << endl;
		exit(EXIT_FAILURE);
	}

	// connect to server on localhost:1234
	ENetAddress address;
	ENetPeer *server;

	string ip_addr;
	cout << "IP Address: ";
	cin >> ip_addr;

	enet_address_set_host( &address, ip_addr.c_str() );
	address.port = 4321;

	server = enet_host_connect( client, &address, 2, 0 );
	
	if( !server )
	{
		cerr << "No more available outgoing connections available" << endl;
		exit(EXIT_FAILURE);
	}

	// wait for connection to complete
	ENetEvent event;
	if( enet_host_service( client, &event, 5000 ) > 0 &&
			event.type == ENET_EVENT_TYPE_CONNECT  )
	{
		cout << "Connection to server succeeded" << endl;

		// create a packet to send
		int value = 0;
		ENetPacket *packet = enet_packet_create( &value, sizeof(value), ENET_PACKET_FLAG_RELIABLE );

		enet_peer_send( server, 0, packet );

		while( !quit && enet_host_service( client, &event, 0 ) >= 0 )
		{
			switch( event.type )
			{
				case ENET_EVENT_TYPE_CONNECT:
					// new connection request, shouldn't happen
					cerr << "New connection request, wtf?" << endl;
					break;

				case ENET_EVENT_TYPE_RECEIVE:
					// new data from a server
					value = *(int*)event.packet->data;
					cout << "Packet received val = " << value << endl;

					sleep( 1 );

					// increment and resend
					value++;
					packet = enet_packet_create( &value, sizeof(value), ENET_PACKET_FLAG_RELIABLE );

					enet_peer_send( event.peer, 0, packet );
					break;

				case ENET_EVENT_TYPE_DISCONNECT:
					// server disconnect
					cout << "Server "  << event.peer->data << " disconnect" << endl;

					quit = true;
					break;
				case ENET_EVENT_TYPE_NONE:
					// nothing happened
					break;
			}
		}
	}
	else
	{
		enet_peer_reset( server );

		cerr << "Connection to server refused" << endl;
		exit(EXIT_FAILURE);
	}

	enet_peer_disconnect( server, 0 );

	while (enet_host_service (client, & event, 3000) > 0)
	{
		switch (event.type)
		{
			case ENET_EVENT_TYPE_DISCONNECT:
				cout << "Disconnection succeeded." << endl;
				break;
			default:
				break;
		}
	}

	enet_peer_reset( server );
	enet_host_destroy( client );

	return 0;
}
