/*
 * File:	ClientApp.h
 */
#ifndef __ClientApp_h_
#define __ClientApp_h_
 
#include "BaseApplication.h"
#include "terrainPager.h"

#include <OgreCamera.h>
#include <OgreEntity.h>
#include <OgreViewport.h>
#include <OgreSceneManager.h>
#include <OgreRenderWindow.h>

#include <OISEvents.h>
#include <OISInputManager.h>
#include <OISKeyboard.h>
#include <OISMouse.h>

#include <SdkTrays.h>
#include <cameraMan.h>

#include <PolyVoxCore/SimpleInterface.h>
#include <PolyVoxCore/LargeVolume.h>
#include <PolyVoxCore/Material.h>

class ClientApp : public Ogre::FrameListener, public Ogre::WindowEventListener, public OIS::KeyListener, public OIS::MouseListener, OgreBites::SdkTrayListener, public BaseApplication
{
private:
	TerrainPager *terrain;
	Ogre::SceneNode *mCursor;

	void doTerrainUpdate();
	void createCursor( float radius );

public:
    ClientApp(void);
    virtual ~ClientApp(void);
 
    virtual void go(void);

protected:
    virtual void createScene(void);
    virtual void createFrameListener(void);
    virtual void destroyScene(void);
    virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);
	virtual bool frameStarted(const Ogre::FrameEvent &evt);
	virtual bool frameEnded(const Ogre::FrameEvent &evt);

	// OIS::KeyListener
    virtual bool keyPressed( const OIS::KeyEvent& evt );
    virtual bool keyReleased( const OIS::KeyEvent& evt );

    // OIS::MouseListener
    virtual bool mouseMoved( const OIS::MouseEvent& evt );
    virtual bool mousePressed( const OIS::MouseEvent& evt, OIS::MouseButtonID id );
    virtual bool mouseReleased( const OIS::MouseEvent& evt, OIS::MouseButtonID id );

    virtual bool setup();
    virtual bool setupNetwork();
    virtual bool configure(void);
    virtual void chooseSceneManager(void);
    virtual void createCamera(void);
    virtual void createViewports(void);
    virtual void setupResources(void);
    virtual void createResourceListener(void);
    virtual void loadResources(void);

    // Ogre::WindowEventListener
    //Adjust mouse clipping area
    virtual void windowResized(Ogre::RenderWindow* rw);
    //Unattach OIS before window shutdown (very important under Linux)
    virtual void windowClosed(Ogre::RenderWindow* rw);

	ENetPeer *server;
	ENetHost *client;

    Ogre::Camera* mCamera;
    Ogre::SceneManager* mSceneMgr;
    Ogre::RenderWindow* mWindow;
    Ogre::String mResourcesCfg;

    // OgreBites
    OgreBites::SdkTrayManager* mTrayMgr;
    CameraMan* mCameraMan;       // basic camera controller
    OgreBites::ParamsPanel* mDetailsPanel;     // sample details panel
    bool mCursorWasVisible;                    // was cursor visible before dialog appeared
    bool mShutDown;

    //OIS Input devices
    OIS::InputManager* mInputManager;
    OIS::Mouse*    mMouse;
    OIS::Keyboard* mKeyboard;
};
 
#endif // #ifndef __ClientApp_h_