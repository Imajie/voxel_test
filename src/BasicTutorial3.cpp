/*
-----------------------------------------------------------------------------
Filename:    BasicTutorial3.cpp
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
 
#include <vector>
#include <cassert>

#include "perlinNoise.h"

#include "BasicTutorial3.h"
#include "PolyVoxCore/SimpleInterface.h"
#include "PolyVoxCore/CubicSurfaceExtractor.h"
#include "PolyVoxCore/SurfaceMesh.h"
#include "PolyVoxCore/Raycast.h"

#define VOXEL_SCALE 10.0
#define MODIFY_RADIUS 5.0

using namespace std;

void BasicTutorial3::doTerrainUpdate()
{
	terrain->regenerateMesh( mCamera->getPosition() );
}

void createSphereInVolume(PolyVox::LargeVolume<PolyVox::Material<uint8_t> >& volData, float fRadius, PolyVox::Vector3DInt32 _center, uint8_t material)
{
	PolyVox::Vector3DFloat center( _center.getX(), _center.getY(), _center.getZ() );

	//This vector hold the position of the center of the volume
	PolyVox::Region volRegion(volData.getEnclosingRegion());

	PolyVox::Vector3DFloat v3dVolCenter( (volRegion.getLowerCorner() + volRegion.getUpperCorner()) );
	v3dVolCenter /= 2.0f;

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

					if( volData.getEnclosingRegion().containsPoint(point) )
					{
						//Get the old voxel
						PolyVox::Material<uint8_t>  voxel = volData.getVoxelAt(point);

						voxel.setMaterial(material);

						//Wrte the voxel value into the volume
						volData.setVoxelAt( point, voxel);
					}
				}
			}
		}
	}
}

void BasicTutorial3::createCursor( float radius )
{
	radius *= VOXEL_SCALE;
	radius -= 1.0;

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
BasicTutorial3::BasicTutorial3(void)
	: terrain(NULL)
{
}
//-------------------------------------------------------------------------------------
BasicTutorial3::~BasicTutorial3(void)
{
}
//-------------------------------------------------------------------------------------
void BasicTutorial3::destroyScene(void)
{
}
//-------------------------------------------------------------------------------------
void BasicTutorial3::createScene(void)
{
	createCursor(MODIFY_RADIUS);

	//mCamera->setPosition(Ogre::Vector3(1683, 50, 2116));
	//mCamera->lookAt(Ogre::Vector3(1963, 50, 1660));
	mCamera->setPosition(Ogre::Vector3(0, 50, 0));
	mCamera->lookAt(Ogre::Vector3(100, 50, 0));
	mCamera->setNearClipDistance(0.1);
	mCamera->setFarClipDistance(50000);

	if (mRoot->getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_INFINITE_FAR_PLANE))
	{
		mCamera->setFarClipDistance(0);   // enable infinite far clip distance if we can
	}

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

	terrain = new TerrainPager( mSceneMgr, ogreNode );

	doTerrainUpdate();

	Ogre::Plane plane;
	plane.d = 100;
	plane.normal = Ogre::Vector3::NEGATIVE_UNIT_Y;

	mSceneMgr->setSkyPlane(true, plane, "Examples/CloudySky", 500, 20, true, 0.5, 150, 150);
}
//-------------------------------------------------------------------------------------
void BasicTutorial3::createFrameListener(void)
{
	BaseApplication::createFrameListener();

	// set event listener
	mMouse->setEventCallback(this);
	mKeyboard->setEventCallback(this);
}
//-------------------------------------------------------------------------------------
bool BasicTutorial3::frameStarted(const Ogre::FrameEvent &evt)
{
	//terrain->lock();
	return true;
}
//-------------------------------------------------------------------------------------
bool BasicTutorial3::frameEnded(const Ogre::FrameEvent &evt)
{
	//terrain->unlock();
	return true;
}
//-------------------------------------------------------------------------------------
bool BasicTutorial3::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	bool ret = BaseApplication::frameRenderingQueued(evt);

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

	return ret;
}

// OIS::KeyListener
bool BasicTutorial3::keyPressed( const OIS::KeyEvent& evt )
{
	//doTerrainUpdate();
	bool ret = BaseApplication::keyPressed( evt );
	return ret;
}

bool BasicTutorial3::keyReleased( const OIS::KeyEvent& evt )
{
	//doTerrainUpdate();
	mCameraMan->injectKeyUp(evt);
	return true;
}

// OIS::MouseListener
bool BasicTutorial3::mouseMoved( const OIS::MouseEvent& evt )
{
	mCameraMan->injectMouseMove(evt);

	return true;
}

bool BasicTutorial3::mousePressed( const OIS::MouseEvent& evt, OIS::MouseButtonID id )
{
	if( id == OIS::MB_Left || id == OIS::MB_Right )
	{
		PolyVox::Vector3DFloat start(mCamera->getPosition().x/VOXEL_SCALE, mCamera->getPosition().y/VOXEL_SCALE, mCamera->getPosition().z/VOXEL_SCALE );
		PolyVox::Vector3DFloat dir(mCamera->getDirection().x, mCamera->getDirection().y, mCamera->getDirection().z );

		dir.normalise();
		dir *= 1000;

		PolyVox::RaycastResult result;

		terrain->raycast( start, dir, result );

		if( result.foundIntersection )
		{
			if( id == OIS::MB_Left )
			{
				//createSphereInVolume( volume, MODIFY_RADIUS, result.previousVoxel, 1 );
			}
			else if( id == OIS::MB_Right )
			{
				//createSphereInVolume( volume, MODIFY_RADIUS, result.intersectionVoxel, 0 );
			}

			doTerrainUpdate();
		}
	}

	mCameraMan->injectMouseDown(evt, id);
	return true;
}

bool BasicTutorial3::mouseReleased( const OIS::MouseEvent& evt, OIS::MouseButtonID id )
{
	mCameraMan->injectMouseUp(evt, id);
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
			// Create application object
			initPerlinNoise();

			BasicTutorial3 app;

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
