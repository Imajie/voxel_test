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

	while( !quit )
	{
		//Pump messages in all registered RenderWindow windows
		Ogre::WindowEventUtilities::messagePump();

		// handle network
		ENetEvent event;
		//ENetPacket *packet;

		if( enet_host_service( server, &event, 0 ) >= 0 )
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
					}
					else
					{
						// new data from a existing client
						//packet = event.packet;
					}
					break;

				case ENET_EVENT_TYPE_DISCONNECT:
					// client disconnect
					cout << "Client "  << (char*)event.peer->data << " disconnect" << endl;

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
