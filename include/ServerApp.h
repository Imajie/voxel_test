/*
 * File:	ServerApp.h
 * Author:	James Letendre
 */
#ifndef __ServerApp_h_
#define __ServerApp_h_
 
#include "BaseApplication.h"
#include "terrainPager.h"

#include "PlayerEntity.h"

#include <PolyVoxCore/SimpleInterface.h>
#include <PolyVoxCore/LargeVolume.h>
#include <PolyVoxCore/Material.h>

#include <vector>

class ServerApp : public BaseApplication
{
private:
	TerrainPager *terrain;

public:
    ServerApp(void);
    virtual ~ServerApp(void);
 
    virtual void go(void);

protected:
    virtual bool setup();
    virtual bool setupNetwork();

	ENetHost *server;
	std::vector<PlayerEntity> players;
};
 
#endif // #ifndef __ServerApp_h_
