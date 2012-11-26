/*
 * File:	OgreApp.h
 */
#ifndef __OgreApp_h_
#define __OgreApp_h_
 
#include "BaseApplication.h"

#include "terrainPager.h"
 
#include <PolyVoxCore/SimpleInterface.h>
#include <PolyVoxCore/LargeVolume.h>
#include <PolyVoxCore/Material.h>

class OgreApp : public BaseApplication
{
private:
	TerrainPager *terrain;
	Ogre::SceneNode *mCursor;

	void doTerrainUpdate();
	void createCursor( float radius );

public:
    OgreApp(void);
    virtual ~OgreApp(void);
 
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
};
 
#endif // #ifndef __OgreApp_h_
