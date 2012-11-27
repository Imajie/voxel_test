/*
 * File:	ServerApp.cpp
 * Author:	James Letendre
 *
 */

#include <vector>
#include <cassert>

#include "perlinNoise.h"

#include "ServerApp.h"
#include "PolyVoxCore/SimpleInterface.h"
#include "PolyVoxCore/CubicSurfaceExtractor.h"
#include "PolyVoxCore/SurfaceMesh.h"
#include "PolyVoxCore/Raycast.h"

#include <OgreWindowEventUtilities.h>

#define VOXEL_SCALE 1.0

using namespace std;

bool quit = false;

void kill_sig( int code )
{
	quit = true;
}

//-------------------------------------------------------------------------------------
ServerApp::ServerApp(void) : terrain(NULL)
{
}
//-------------------------------------------------------------------------------------
ServerApp::~ServerApp(void)
{
	if(terrain) delete terrain;
}
//-------------------------------------------------------------------------------------
bool ServerApp::setupNetwork(void)
{
	// initialize ENet
	if( enet_initialize() != 0 )
	{
		cerr << "Error initializing ENet" << endl;
		return false;
	}

	// add hook to run on exit
	atexit(enet_deinitialize);

	ENetAddress address;

	address.host = ENET_HOST_ANY;
	address.port = 4321;

	cout << "Creating server" << endl;
	// create server with max 32 clients, 2 channels and now bandwidth limits
	server = enet_host_create( &address, 32, 2, 0, 0 );

	if( !server )
	{
		cerr << "Error creating ENet server" << endl;
		return false;
	}

	return true;
}
//-------------------------------------------------------------------------------------
void ServerApp::go(void)
{
	if (!setup())
		return;

	terrain = new TerrainPager(NULL, NULL, this);
	
	while( !quit )
	{
		//Pump messages in all registered RenderWindow windows
		Ogre::WindowEventUtilities::messagePump();

		// handle network
		ENetEvent event;
		std::string playerName;

		if( enet_host_service( server, &event, 0 ) > 0 )
		{
			switch( event.type )
			{
				case ENET_EVENT_TYPE_CONNECT:
					// new connection request
					cout << "New connection request from " << event.peer->address.host << ":" << event.peer->address.port << endl;

					event.peer->data = (void*)"";
					break;

				case ENET_EVENT_TYPE_RECEIVE:
					if( strcmp( (char*)event.peer->data, "" ) == 0 )
					{
						// new client
						event.peer->data = strdup( (char*)event.packet->data );

						cout << "Client " << event.peer->address.host << " known as " << (char*)event.peer->data << endl;

						playerName = (char*)event.peer->data;
						players.push_back( PlayerEntity( playerName ) );

						// broadcast to all clients
						Packet joinPacket;
						joinPacket.type = PLAYER_CONNECT_PACKET;
						for( char c : playerName )
						{
							joinPacket.data.push_back(c);
						}

						joinPacket.broadcast(server, ENET_PACKET_FLAG_RELIABLE);
					}
					else
					{
						// new data from a existing client
						Packet recvPacket;
						Packet *pkt;
						recvPacket.unserialize( event.packet->data, event.packet->dataLength );

						switch( recvPacket.type )
						{
							case PLAYER_CONNECT_PACKET:
								break;
							case PLAYER_DISCONNECT_PACKET:
								break;
							case PLAYER_MOVE_PACKET:
								break;

							case TERRAIN_REQUEST_PACKET:
								pkt = terrain->request( &recvPacket );
								pkt->send( event.peer, ENET_PACKET_FLAG_RELIABLE );
								delete pkt;
								break;
							case TERRAIN_RESPONSE_PACKET:
								// not used in server
								break;
							case TERRAIN_UPDATE_PACKET:
								break;

							default:
								break;
						}
					}
					break;

				case ENET_EVENT_TYPE_DISCONNECT:
					// client disconnect
					cout << "Client "  << (char*)event.peer->data << " disconnect" << endl;

					// delete the player
					playerName = (char*)event.peer->data;
					for( size_t i = 0; i < players.size(); i++ )
					{
						if( playerName == players[i].getName() )
						{
							Packet dropPacket;
							dropPacket.type = PLAYER_DISCONNECT_PACKET;
							for( char c : playerName )
							{
								dropPacket.data.push_back(c);
							}
							
							dropPacket.broadcast(server, ENET_PACKET_FLAG_RELIABLE);

							players.erase( players.begin() + i );
							break;
						}
					}

					free(event.peer->data);
					event.peer->data = NULL;

					break;
				case ENET_EVENT_TYPE_NONE:
					// nothing happened
					break;
			}
		}
	}

}
//-------------------------------------------------------------------------------------
bool ServerApp::setup(void)
{
	if( !BaseApplication::setup() )
		return false;

	setupNetwork();

	return true;
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
		int main(int argc, char *argv[])
#endif
		{
			signal( SIGKILL, kill_sig );
			signal( SIGTERM, kill_sig );
			signal( SIGINT,  kill_sig );

			// Create application object
			initPerlinNoise();

			ServerApp app;

			try {
				app.go();
			} catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
				std::cerr << "An exception has occured: " <<
					e.getFullDescription().c_str() << std::endl;
#endif
			}

			return 0;
		}

#ifdef __cplusplus
}
#endif
