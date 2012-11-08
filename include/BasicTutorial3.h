/*
-----------------------------------------------------------------------------
Filename:    BasicTutorial3.h
-----------------------------------------------------------------------------
 
This source file is part of the
   ___                 __    __ _ _    _ 
  /___\__ _ _ __ ___  / / /\ \ (_) | _(_)
 //  // _` | '__/ _ \ \ \/  \/ / | |/ / |
/ \_// (_| | | |  __/  \  /\  /| |   <| |
\___/ \__, |_|  \___|   \/  \/ |_|_|\_\_|
      |___/                              
      Tutorial Framework
      http://www.ogre3d.org/tikiwiki/
-----------------------------------------------------------------------------
*/
#ifndef __BasicTutorial3_h_
#define __BasicTutorial3_h_
 
#include "BaseApplication.h"

#include "terrainPager.h"
 
#include <PolyVoxCore/SimpleInterface.h>
#include <PolyVoxCore/LargeVolume.h>
#include <PolyVoxCore/Material.h>

class BasicTutorial3 : public BaseApplication
{
private:
	TerrainPager *terrain;
	Ogre::SceneNode *mCursor;

	void doTerrainUpdate();
	void createCursor( float radius );

public:
    BasicTutorial3(void);
    virtual ~BasicTutorial3(void);
 
protected:
    virtual void createScene(void);
    virtual void createFrameListener(void);
    virtual void destroyScene(void);
    virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);

	// OIS::KeyListener
    virtual bool keyPressed( const OIS::KeyEvent& evt );
    virtual bool keyReleased( const OIS::KeyEvent& evt );

    // OIS::MouseListener
    virtual bool mouseMoved( const OIS::MouseEvent& evt );
    virtual bool mousePressed( const OIS::MouseEvent& evt, OIS::MouseButtonID id );
    virtual bool mouseReleased( const OIS::MouseEvent& evt, OIS::MouseButtonID id );
};
 
#endif // #ifndef __BasicTutorial3_h_
