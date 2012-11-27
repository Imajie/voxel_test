/*
-----------------------------------------------------------------------------
Filename:    BaseApplication.cpp
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
#include "BaseApplication.h"

//-------------------------------------------------------------------------------------
BaseApplication::BaseApplication(void)
    : mRoot(0),
    mPluginsCfg(Ogre::StringUtil::BLANK)
{
}

//-------------------------------------------------------------------------------------
BaseApplication::~BaseApplication(void)
{
    delete mRoot;
}

//-------------------------------------------------------------------------------------
bool BaseApplication::setup(void)
{
#ifdef _DEBUG
	mPluginsCfg = "plugins_d.cfg";
#else
	mPluginsCfg = "plugins.cfg";
#endif

    mRoot = new Ogre::Root(mPluginsCfg);
    return true;
}

