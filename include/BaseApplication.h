/*
-----------------------------------------------------------------------------
Filename:    BaseApplication.h
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
#ifndef __BaseApplication_h_
#define __BaseApplication_h_

#include <OgreRoot.h>
#include <OgreLogManager.h>
#include <OgreConfigFile.h>


class BaseApplication {
public:
    BaseApplication(void);
    virtual ~BaseApplication(void);

protected:
	bool setup(void);

    Ogre::Root *mRoot;
    Ogre::String mPluginsCfg;
};

#endif // #ifndef __BaseApplication_h_
