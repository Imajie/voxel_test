/*
 * File:	ClientApp.cpp
 *
 */

#include <vector>
#include <cassert>

#include "perlinNoise.h"

#include "ClientApp.h"
#include "PolyVoxCore/SimpleInterface.h"
#include "PolyVoxCore/CubicSurfaceExtractor.h"
#include "PolyVoxCore/SurfaceMesh.h"
#include "PolyVoxCore/Raycast.h"

#include <OgreWindowEventUtilities.h>

#define VOXEL_SCALE 1.0
#define MODIFY_RADIUS 0.5

using namespace std;

bool quit = false;

void kill_sig( int code )
{
	quit = true;
}

// instance
ClientApp *ClientApp::self = NULL;

void ClientApp::doTerrainUpdate()
{
	terrain->regenerateMesh( mCamera->getPosition() );
}

void createSphereInVolume(TerrainPager* volData, float fRadius, PolyVox::Vector3DInt32 _center, uint8_t material)
{
	PolyVox::Vector3DFloat center( _center.getX(), _center.getY(), _center.getZ() );

	//This vector hold the position of the center of the volume
	PolyVox::Region volRegion(volData->getEnclosingRegion());

	//This three-level for loop iterates over every voxel in the volume
	for (int z = max(center.getZ() - fRadius, (float)volRegion.getLowerCorner().getZ()); z < min(center.getZ() + fRadius, (float)volRegion.getUpperCorner().getZ()); z++)
	{
		for (int y = max(center.getY() - fRadius, (float)volRegion.getLowerCorner().getY()); y < min(center.getY() + fRadius, (float)volRegion.getUpperCorner().getY()); y++)
		{
			for (int x = max(center.getX() - fRadius, (float)volRegion.getLowerCorner().getX()); x < min(center.getX() + fRadius, (float)volRegion.getUpperCorner().getX()); x++)
			{
				//Store our current position as a vector...
				PolyVox::Vector3DFloat v3dCurrentPos(x,y,z);
				//And compute how far the current position is from the center of the volume
				float fDistToCenter = (v3dCurrentPos - center).length();

				//If the current voxel is less than 'radius' units from the center then we make it solid.
				if(fDistToCenter <= fRadius)
				{
					PolyVox::Vector3DInt32 point(x,y,z);

					if( volRegion.containsPoint(point) )
					{
						//Get the old voxel
						PolyVox::Material<uint8_t>  voxel = volData->getVoxelAt(point);

						voxel.setMaterial(material);

						//Wrte the voxel value into the volume
						volData->setVoxelAt( point, voxel);
					}
				}
			}
		}
	}
}

void ClientApp::createCursor( float radius )
{
	radius *= VOXEL_SCALE;

	// Assuming scene_mgr is your SceneManager.
	Ogre::ManualObject * circle = mSceneMgr->createManualObject("debugCursor");

	// accuracy is the count of points (and lines).
	// Higher values make the circle smoother, but may slowdown the performance.
	// The performance also is related to the count of circles.
	float const accuracy = 35;

	circle->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_STRIP);

	unsigned point_index = 0;
	for(float theta = 0; theta <= 2 * M_PI; theta += M_PI / accuracy) {
		circle->position(radius * cos(theta), 0, radius * sin(theta));
		circle->index(point_index++);
	}
	for(float theta = 0; theta <= 2 * M_PI; theta += M_PI / accuracy) {
		circle->position(radius * cos(theta), radius * sin(theta), 0);
		circle->index(point_index++);
	}
	circle->index(0); // Rejoins the last point to the first.

	circle->end();

	mCursor = mSceneMgr->getRootSceneNode()->createChildSceneNode("debugCursor");
	mCursor->attachObject(circle);
}


//-------------------------------------------------------------------------------------
ClientApp::ClientApp(void) : terrain(NULL),
	mCamera(0),
	mSceneMgr(0),
	mWindow(0),
	mResourcesCfg(Ogre::StringUtil::BLANK),
	mTrayMgr(0),
	mCameraMan(0),
	mDetailsPanel(0),
	mCursorWasVisible(false),
	mShutDown(false),
	mInputManager(0),
	mMouse(0),
	mKeyboard(0)
{
}
//-------------------------------------------------------------------------------------
ClientApp::~ClientApp(void)
{
	if (mTrayMgr) delete mTrayMgr;
	if (mCameraMan) delete mCameraMan;

	//Remove ourself as a Window listener
	Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
	windowClosed(mWindow);
}
//-------------------------------------------------------------------------------------
bool ClientApp::configure(void)
{
	// Show the configuration dialog and initialise the system
	// You can skip this and use root.restoreConfig() to load configuration
	// settings if you were sure there are valid ones saved in ogre.cfg
	if(mRoot->showConfigDialog())
	{
		// If returned true, user clicked OK so initialise
		// Here we choose to let the system create a default rendering window by passing 'true'
		mWindow = mRoot->initialise(true, "TutorialApplication Render Window");

		return true;
	}
	else
	{
		return false;
	}
}
//-------------------------------------------------------------------------------------
void ClientApp::chooseSceneManager(void)
{
	// Get the SceneManager, in this case a generic one
	mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC);
}
//-------------------------------------------------------------------------------------
void ClientApp::createCamera(void)
{
	// Create the camera
	mCamera = mSceneMgr->createCamera("PlayerCam");

	// Position it at 500 in Z direction
	mCamera->setPosition(Ogre::Vector3(0,0,80));
	// Look back along -Z
	mCamera->lookAt(Ogre::Vector3(0,0,-300));
	mCamera->setNearClipDistance(5);

	mCameraMan = new CameraMan(mCamera);   // create a default camera controller

}
//-------------------------------------------------------------------------------------
void ClientApp::createFrameListener(void)
{
	Ogre::LogManager::getSingletonPtr()->logMessage("*** Initializing OIS ***");
	OIS::ParamList pl;
	size_t windowHnd = 0;
	std::ostringstream windowHndStr;

	mWindow->getCustomAttribute("WINDOW", &windowHnd);
	windowHndStr << windowHnd;
	pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

	mInputManager = OIS::InputManager::createInputSystem( pl );

	mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject( OIS::OISKeyboard, true ));
	mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject( OIS::OISMouse, true ));

	mMouse->setEventCallback(this);
	mKeyboard->setEventCallback(this);

	//Set initial mouse clipping size
	windowResized(mWindow);

	//Register as a Window listener
	Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

	mTrayMgr = new OgreBites::SdkTrayManager("InterfaceName", mWindow, mMouse, this);
	mTrayMgr->showFrameStats(OgreBites::TL_BOTTOMLEFT);
	mTrayMgr->showLogo(OgreBites::TL_BOTTOMRIGHT);
	mTrayMgr->hideCursor();

	// create a params panel for displaying sample details
	Ogre::StringVector items;
	items.push_back("cam.pX");
	items.push_back("cam.pY");
	items.push_back("cam.pZ");
	items.push_back("");
	items.push_back("cam.oW");
	items.push_back("cam.oX");
	items.push_back("cam.oY");
	items.push_back("cam.oZ");
	items.push_back("");
	items.push_back("Filtering");
	items.push_back("Poly Mode");

	mDetailsPanel = mTrayMgr->createParamsPanel(OgreBites::TL_NONE, "DetailsPanel", 200, items);
	mDetailsPanel->setParamValue(9, "Bilinear");
	mDetailsPanel->setParamValue(10, "Solid");
	mDetailsPanel->hide();

	mRoot->addFrameListener(this);
}
//-------------------------------------------------------------------------------------
void ClientApp::destroyScene(void)
{
}
//-------------------------------------------------------------------------------------
void ClientApp::createViewports(void)
{
	// Create one viewport, entire window
	Ogre::Viewport* vp = mWindow->addViewport(mCamera);
	vp->setBackgroundColour(Ogre::ColourValue(0,0,0));

	// Alter the camera aspect ratio to match the viewport
	mCamera->setAspectRatio(
			Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));
}
//-------------------------------------------------------------------------------------
void ClientApp::setupResources(void)
{
	// Load resource paths from config file
	Ogre::ConfigFile cf;
	cf.load(mResourcesCfg);

	// Go through all sections & settings in the file
	Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

	Ogre::String secName, typeName, archName;
	while (seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
		Ogre::ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i)
		{
			typeName = i->first;
			archName = i->second;
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
					archName, typeName, secName);
		}
	}
}
//-------------------------------------------------------------------------------------
void ClientApp::createResourceListener(void)
{

}
//-------------------------------------------------------------------------------------
void ClientApp::loadResources(void)
{
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}
//-------------------------------------------------------------------------------------
bool ClientApp::setupNetwork(void)
{
	// initialize ENet
	if( enet_initialize() != 0 )
	{
		cerr << "Error initializing ENet" << endl;
		exit(EXIT_FAILURE);
	}

	// add hook to run on exit
	atexit(enet_deinitialize);

	// create an ENet client, limit 1 connection (to server), 2 channels, no bandwidth limits
	client = enet_host_create( NULL, 1, 2, 0, 0 );

	if( !client )
	{
		cerr << "Error creating ENet client" << endl;
		exit(EXIT_FAILURE);
	}

	// connect to server on localhost:1234
	ENetAddress address;

	string ip_addr;
	cout << "IP Address: ";
	cin >> ip_addr;

	string userName;
	cout << "User name: ";
	cin >> userName;

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

		ENetPacket *packet = enet_packet_create( userName.c_str(), userName.length()+1, ENET_PACKET_FLAG_RELIABLE );

		enet_peer_send( server, 0, packet );
		enet_host_flush( client );

		return true;
	}
	return false;
}
//-------------------------------------------------------------------------------------
void ClientApp::go(void)
{
#ifdef _DEBUG
	mResourcesCfg = "resources_d.cfg";
#else
	mResourcesCfg = "resources.cfg";
#endif
	if (!setup())
		return;

	while( !quit )
	{
		//Pump messages in all registered RenderWindow windows
		Ogre::WindowEventUtilities::messagePump();

		// handle OGRE
		if( mWindow->isClosed() )
			break;

		if (!mRoot->renderOneFrame())
			break;
	
		// handle network
		ENetEvent event;
		std::string playerName;
		Packet recvPacket;

		if( enet_host_service( client, &event, 0 ) > 0 )
		{
			switch( event.type )
			{
				case ENET_EVENT_TYPE_CONNECT:
					// not used
					break;

				case ENET_EVENT_TYPE_RECEIVE:
					// new data from server
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
							// not used in client
							break;
						case TERRAIN_RESPONSE_PACKET:
							terrain->unserialize( &recvPacket );
							break;
						case TERRAIN_UPDATE_PACKET:
							break;

						default:
							break;
					}
					break;

				case ENET_EVENT_TYPE_DISCONNECT:
					// server disconnect
					cout << "Server disconnect" << endl;
					quit = true;
					break;
				case ENET_EVENT_TYPE_NONE:
					// nothing happened
					break;
			}
		}
		
	}

	// clean up
	destroyScene();

	enet_peer_disconnect( server, 0 );

	ENetEvent event;
	bool disconnect = false;
	while (!disconnect && enet_host_service (client, & event, 3000) > 0)
	{
		switch (event.type)
		{
			case ENET_EVENT_TYPE_DISCONNECT:
				cout << "Disconnection succeeded." << endl;
				disconnect = true;
				break;
			default:
				break;
		}
	}

	enet_peer_reset( server );
	enet_host_destroy( client );

}
//-------------------------------------------------------------------------------------
bool ClientApp::setup(void)
{
	if( !BaseApplication::setup() )
		return false;

	setupNetwork();
	setupResources();

	bool carryOn = configure();
	if (!carryOn) return false;

	chooseSceneManager();
	createCamera();
	createViewports();

	// Set default mipmap level (NB some APIs ignore this)
	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

	// Create any resource listeners (for loading screens)
	createResourceListener();
	// Load resources
	loadResources();

	// Create the scene
	createScene();

	createFrameListener();

	return true;
}
//-------------------------------------------------------------------------------------
void ClientApp::createScene(void)
{
	createCursor(MODIFY_RADIUS);

	mCameraMan->setTopSpeed(2.0);

	mCamera->setPosition(Ogre::Vector3(0, 33, 0));
	mCamera->lookAt(Ogre::Vector3(100, 33, 0));
	mCamera->setNearClipDistance(0.1);
	mCamera->setFarClipDistance(5000);

	// Play with startup Texture Filtering options
	// Note: Pressing T on runtime will discarde those settings
	//  Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(Ogre::TFO_ANISOTROPIC);
	//  Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(7);

	Ogre::Vector3 lightdir(0.55, -0.3, 0.75);
	lightdir.normalise();

	Ogre::Light* light = mSceneMgr->createLight("tstLight");
	light->setType(Ogre::Light::LT_DIRECTIONAL);
	light->setDirection(lightdir);
	light->setDiffuseColour(Ogre::ColourValue::White);
	light->setSpecularColour(Ogre::ColourValue(0.4, 0.4, 0.4));

	mSceneMgr->setAmbientLight(Ogre::ColourValue(0.2, 0.2, 0.2));

	// PolyVox stuff
	Ogre::SceneNode* ogreNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("testnode1", Ogre::Vector3(0, 0, 0));

	terrain = new TerrainPager( mSceneMgr, ogreNode, this );
	mCameraMan->setTerrain(terrain);

	doTerrainUpdate();

	Ogre::Plane plane;
	plane.d = 100;
	plane.normal = Ogre::Vector3::NEGATIVE_UNIT_Y;

	mSceneMgr->setSkyPlane(true, plane, "Examples/CloudySky", 500, 20, true, 0.5, 150, 150);
}
//-------------------------------------------------------------------------------------
bool ClientApp::frameStarted(const Ogre::FrameEvent &evt)
{
	//terrain->lock();
	return true;
}
//-------------------------------------------------------------------------------------
bool ClientApp::frameEnded(const Ogre::FrameEvent &evt)
{
	//terrain->unlock();
	return true;
}
//-------------------------------------------------------------------------------------
bool ClientApp::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	if(mWindow->isClosed())
		return false;

	if(mShutDown)
		return false;

	//Need to capture/update each device
	mKeyboard->capture();
	mMouse->capture();

	mTrayMgr->frameRenderingQueued(evt);

	if (!mTrayMgr->isDialogVisible())
	{
		mCameraMan->frameRenderingQueued(evt);   // if dialog isn't up, then update the camera
		if (mDetailsPanel->isVisible())   // if details panel is visible, then update its contents
		{
			mDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(mCamera->getDerivedPosition().x));
			mDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(mCamera->getDerivedPosition().y));
			mDetailsPanel->setParamValue(2, Ogre::StringConverter::toString(mCamera->getDerivedPosition().z));
			mDetailsPanel->setParamValue(4, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().w));
			mDetailsPanel->setParamValue(5, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().x));
			mDetailsPanel->setParamValue(6, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().y));
			mDetailsPanel->setParamValue(7, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().z));
		}
	}

	if (mWindow->isClosed()) return false;
	if (mShutDown) return false;

	mKeyboard->capture();
	mMouse->capture();
	mTrayMgr->frameRenderingQueued(evt);

	// move cursor
	PolyVox::Vector3DFloat start(mCamera->getPosition().x/VOXEL_SCALE, mCamera->getPosition().y/VOXEL_SCALE, mCamera->getPosition().z/VOXEL_SCALE );
	PolyVox::Vector3DFloat dir(mCamera->getDirection().x, mCamera->getDirection().y, mCamera->getDirection().z );

	dir.normalise();
	dir *= 1000;

	PolyVox::RaycastResult result;
	terrain->raycast( start, dir, result );

	if( result.foundIntersection )
	{
		Ogre::Vector3 intersect( result.previousVoxel.getX()*VOXEL_SCALE, 
				result.previousVoxel.getY()*VOXEL_SCALE, result.previousVoxel.getZ()*VOXEL_SCALE );

		mCursor->setPosition(intersect);
	}

	mCursor->setVisible(result.foundIntersection);

	doTerrainUpdate();

	return true;
}

// OIS::KeyListener
bool ClientApp::keyPressed( const OIS::KeyEvent& arg )
{
	if (mTrayMgr->isDialogVisible()) return true;   // don't process any more keys if dialog is up

	if (arg.key == OIS::KC_F)   // toggle visibility of advanced frame stats
	{
		mTrayMgr->toggleAdvancedFrameStats();
	}
	else if (arg.key == OIS::KC_G)   // toggle visibility of even rarer debugging details
	{
		if (mDetailsPanel->getTrayLocation() == OgreBites::TL_NONE)
		{
			mTrayMgr->moveWidgetToTray(mDetailsPanel, OgreBites::TL_TOPRIGHT, 0);
			mDetailsPanel->show();
		}
		else
		{
			mTrayMgr->removeWidgetFromTray(mDetailsPanel);
			mDetailsPanel->hide();
		}
	}
	else if (arg.key == OIS::KC_T)   // cycle polygon rendering mode
	{
		Ogre::String newVal;
		Ogre::TextureFilterOptions tfo;
		unsigned int aniso;

		switch (mDetailsPanel->getParamValue(9).asUTF8()[0])
		{
			case 'B':
				newVal = "Trilinear";
				tfo = Ogre::TFO_TRILINEAR;
				aniso = 1;
				break;
			case 'T':
				newVal = "Anisotropic";
				tfo = Ogre::TFO_ANISOTROPIC;
				aniso = 8;
				break;
			case 'A':
				newVal = "None";
				tfo = Ogre::TFO_NONE;
				aniso = 1;
				break;
			default:
				newVal = "Bilinear";
				tfo = Ogre::TFO_BILINEAR;
				aniso = 1;
		}

		Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);
		Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(aniso);
		mDetailsPanel->setParamValue(9, newVal);
	}
	else if (arg.key == OIS::KC_P)   // cycle polygon rendering mode
	{
		Ogre::String newVal;
		Ogre::PolygonMode pm;

		switch (mCamera->getPolygonMode())
		{
			case Ogre::PM_SOLID:
				newVal = "Wireframe";
				pm = Ogre::PM_WIREFRAME;
				break;
			case Ogre::PM_WIREFRAME:
				newVal = "Points";
				pm = Ogre::PM_POINTS;
				break;
			default:
				newVal = "Solid";
				pm = Ogre::PM_SOLID;
		}

		mCamera->setPolygonMode(pm);
		mDetailsPanel->setParamValue(10, newVal);
	}
	else if(arg.key == OIS::KC_F5)   // refresh all textures
	{
		Ogre::TextureManager::getSingleton().reloadAll();
	}
	else if (arg.key == OIS::KC_SYSRQ)   // take a screenshot
	{
		mWindow->writeContentsToTimestampedFile("screenshot", ".jpg");
	}
	else if (arg.key == OIS::KC_ESCAPE)
	{
		mShutDown = true;
	}

	if( arg.key == OIS::KC_E || arg.key == OIS::KC_R )
	{
		PolyVox::Vector3DFloat start(mCamera->getPosition().x/VOXEL_SCALE, mCamera->getPosition().y/VOXEL_SCALE, mCamera->getPosition().z/VOXEL_SCALE );
		PolyVox::Vector3DFloat dir(mCamera->getDirection().x, mCamera->getDirection().y, mCamera->getDirection().z );

		dir.normalise();
		dir *= 1000;

		PolyVox::RaycastResult result;

		terrain->raycast( start, dir, result );

		if( result.foundIntersection )
		{
			if( arg.key == OIS::KC_E )
			{
				createSphereInVolume( terrain, MODIFY_RADIUS, result.previousVoxel, 1 );
			}
			else if( arg.key == OIS::KC_R)
			{
				createSphereInVolume( terrain, MODIFY_RADIUS, result.intersectionVoxel, 0 );
			}

			doTerrainUpdate();
		}
	}

	return true;
}

bool ClientApp::keyReleased( const OIS::KeyEvent& evt )
{
	mCameraMan->injectKeyUp(evt);
	return true;
}

// OIS::MouseListener
bool ClientApp::mouseMoved( const OIS::MouseEvent& evt )
{
	if (mTrayMgr->injectMouseMove(evt)) return true;

	mCameraMan->injectMouseMove(evt);
	return true;
}

bool ClientApp::mousePressed( const OIS::MouseEvent& evt, OIS::MouseButtonID id )
{
	if (mTrayMgr->injectMouseDown(evt, id)) return true;

	mCameraMan->injectMouseDown(evt, id);
	return true;
}

bool ClientApp::mouseReleased( const OIS::MouseEvent& evt, OIS::MouseButtonID id )
{
	if (mTrayMgr->injectMouseUp(evt, id)) return true;

	mCameraMan->injectMouseUp(evt, id);
	return true;
}

//Adjust mouse clipping area
void ClientApp::windowResized(Ogre::RenderWindow* rw)
{
	unsigned int width, height, depth;
	int left, top;
	rw->getMetrics(width, height, depth, left, top);

	const OIS::MouseState &ms = mMouse->getMouseState();
	ms.width = width;
	ms.height = height;
}

//Unattach OIS before window shutdown (very important under Linux)
void ClientApp::windowClosed(Ogre::RenderWindow* rw)
{
	//Only close for window that created OIS (the main window in these demos)
	if( rw == mWindow )
	{
		if( mInputManager )
		{
			mInputManager->destroyInputObject( mMouse );
			mInputManager->destroyInputObject( mKeyboard );

			OIS::InputManager::destroyInputSystem(mInputManager);
			mInputManager = 0;
		}
	}
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

			ClientApp *app = ClientApp::getInstance();

			try {
				app->go();
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
