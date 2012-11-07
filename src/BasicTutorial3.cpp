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

#include "BasicTutorial3.h"
#include "terrainGenerator.h"
#include "PolyVoxCore/SimpleVolume.h"
#include "PolyVoxCore/SimpleInterface.h"
#include "PolyVoxCore/CubicSurfaceExtractor.h"
#include "PolyVoxCore/SurfaceExtractor.h"
#include "PolyVoxCore/MaterialDensityPair.h"
#include "PolyVoxCore/SurfaceMesh.h"
#include "PolyVoxCore/Raycast.h"

#define VOXEL_SCALE 10

using namespace std;

void BasicTutorial3::doMeshUpdate()
{
	PolyVox::SurfaceMesh<PolyVox::PositionMaterial> mesh;

	PolyVox::CubicSurfaceExtractor<PolyVox::SimpleVolume, PolyVox::Material<uint8_t>  > suf(&volume, volume.getEnclosingRegion(), &mesh);
	suf.execute();
	const vector<PolyVox::PositionMaterial>& vecVertices = mesh.getVertices();
	const vector<uint32_t>& vecIndices = mesh.getIndices();
	unsigned int uLodLevel = 0;
	int beginIndex = mesh.m_vecLodRecords[uLodLevel].beginIndex;
	int endIndex = mesh.m_vecLodRecords[uLodLevel].endIndex;
	for(int index = beginIndex; index < endIndex; ++index) {
		const PolyVox::PositionMaterial& vertex = vecVertices[vecIndices[index]];

		const PolyVox::Vector3DFloat& v3dVertexPos = vertex.getPosition();

		const PolyVox::Vector3DFloat v3dFinalVertexPos = v3dVertexPos + static_cast<PolyVox::Vector3DFloat>(mesh.m_Region.getLowerCorner());

		ogreMesh->position(v3dFinalVertexPos.getX(), v3dFinalVertexPos.getY(), v3dFinalVertexPos.getZ());

		uint8_t red = ((int)abs(vertex.getPosition().getY())) % 256;
		uint8_t green = ((int)abs(vertex.getPosition().getZ())*3) % 256;
		uint8_t blue = ((int)abs(vertex.getPosition().getX())*5) % 256;

		ogreMesh->colour(red, green, blue);
	}
}

void putHeightMapInVolume(PolyVox::SimpleVolume<PolyVox::Material<uint8_t> > &volume, TerrainGenerator &terrainGen)
{
	PolyVox::Region volRegion(volume.getEnclosingRegion());

	int terrainScaleX = 1;//(volRegion.getUpperCorner().getX() - volRegion.getLowerCorner().getX()) / terrainGen.width();
	int terrainScaleZ = 1;//(volRegion.getUpperCorner().getZ() - volRegion.getLowerCorner().getZ()) / terrainGen.height();

	for (int z = volRegion.getLowerCorner().getZ(); z < volRegion.getUpperCorner().getZ(); z+=terrainScaleZ)
	{
		for (int x = volRegion.getLowerCorner().getX(); x < volRegion.getUpperCorner().getX(); x+=terrainScaleX)
		{
			int xx = (x - volRegion.getLowerCorner().getX())/terrainScaleX;
			int zz = (z - volRegion.getLowerCorner().getZ())/terrainScaleZ;

			if( xx < (int)terrainGen.width() && zz < (int)terrainGen.height() )
			{
				int terrainHeight = terrainGen.get(xx, zz);
				for( int y = volRegion.getLowerCorner().getY(); y < terrainHeight; y++ )
				{
					for( int ix = 0; ix < terrainScaleX; ix++ ) 
					{
						for( int iz = 0; iz < terrainScaleZ; iz++ ) 
						{
							//Get the old voxel
							PolyVox::Material<uint8_t>  voxel = volume.getVoxelAt(x+ix,y,z+iz);

							voxel.setMaterial(1);

							//Wrte the voxel value into the volume
							volume.setVoxelAt(x+ix, y, z+iz, voxel);
						}
					}
				}
			}
		}
	}
}

void createSphereInVolume(PolyVox::SimpleVolume<PolyVox::Material<uint8_t> >& volData, float fRadius, PolyVox::Vector3DFloat center)
{
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
					//Our new density value
					uint8_t uDensity = PolyVox::Material<uint8_t> ::getMaxDensity();

					//Get the old voxel
					PolyVox::Material<uint8_t>  voxel = volData.getVoxelAt(x,y,z);

					//Modify the density
					voxel.setDensity(uDensity);

					//Wrte the voxel value into the volume
					volData.setVoxelAt(x, y, z, voxel);
				}
			}
		}
	}
}


//-------------------------------------------------------------------------------------
BasicTutorial3::BasicTutorial3(void)
	: volume( PolyVox::Region(
				PolyVox::Vector3DInt32(0, -64, 0),
				PolyVox::Vector3DInt32(64, 64, 64)
				)
			)
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
	Ogre::Timer timer;

	mCamera->setPosition(Ogre::Vector3(1683, 50, 2116));
	//mCamera->lookAt(Ogre::Vector3(1963, 50, 1660));
	mCamera->lookAt(Ogre::Vector3(0, 0, 0));
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
	// create something to draw the PolyVox stuff to
	ogreMesh = mSceneMgr->createManualObject("PolyVox Mesh");
	// YES we do intend to change the mesh later -.-
	ogreMesh->setDynamic(true);

	Ogre::SceneNode* ogreNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("testnode1", Ogre::Vector3(0, 0, 0));
	ogreNode->attachObject(ogreMesh);
	ogreNode->setScale( Ogre::Vector3(VOXEL_SCALE, VOXEL_SCALE, VOXEL_SCALE) );

	// volume is setup so
	// now add some data to it
	TerrainGenerator terrainGen( 512, 40 );

	std::cout << "Gen terrain: " << std::endl;

	putHeightMapInVolume(volume, terrainGen);

	//createSphereInVolume(volume, 100.0, PolyVox::Vector3DFloat(0, 0, 0));

	ogreMesh->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);
	{
		doMeshUpdate();
	}
	ogreMesh->end();

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
bool BasicTutorial3::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	bool ret = BaseApplication::frameRenderingQueued(evt);

	if (mWindow->isClosed()) return false;
	if (mShutDown) return false;

	mKeyboard->capture();
	mMouse->capture();
	mTrayMgr->frameRenderingQueued(evt);

	return ret;
}

// OIS::KeyListener
bool BasicTutorial3::keyPressed( const OIS::KeyEvent& evt )
{
	switch(evt.key)
	{
		case OIS::KC_ESCAPE:
			mShutDown = true;
			break;
		default:
			break;
	}

	mCameraMan->injectKeyDown(evt);
	return true;
}

bool BasicTutorial3::keyReleased( const OIS::KeyEvent& evt )
{
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
	if( id == OIS::MB_Left )
	{
		// left pressed, lets make a voxel!!!
		PolyVox::Vector3DFloat start(mCamera->getPosition().x/VOXEL_SCALE, mCamera->getPosition().y/VOXEL_SCALE, mCamera->getPosition().z/VOXEL_SCALE );
		PolyVox::Vector3DFloat dir(mCamera->getDirection().x, mCamera->getDirection().y, mCamera->getDirection().z );

		dir.normalise();
		dir *= 1000;

		cout << "start = (" << start.getX() << ", " << start.getY() << ", " << start.getZ() << ")" << endl;
		cout << "dir = (" << dir.getX() << ", " << dir.getY() << ", " << dir.getZ() << ")" << endl;
		cout << "volume = L(" << volume.getEnclosingRegion().getLowerCorner().getX() << ", " << 
			volume.getEnclosingRegion().getLowerCorner().getY() << ", " << volume.getEnclosingRegion().getLowerCorner().getZ() << ")" << endl;
		cout << "volume = U(" << volume.getEnclosingRegion().getUpperCorner().getX() << ", " << 
			volume.getEnclosingRegion().getUpperCorner().getY() << ", " << volume.getEnclosingRegion().getUpperCorner().getZ() << ")" << endl;

		PolyVox::RaycastResult result;
		PolyVox::Raycast<PolyVox::SimpleVolume, PolyVox::Material<uint8_t> > caster(&volume, start, dir, result);

		caster.execute();

		if( result.foundIntersection )
		{
			cout << "Found intersection" << endl;
			PolyVox::Material<uint8_t> voxel = volume.getVoxelAt(result.previousVoxel);
			voxel.setMaterial(1);
			volume.setVoxelAt(result.previousVoxel, voxel);

			ogreMesh->beginUpdate(0);
			{
				doMeshUpdate();
			}
			ogreMesh->end();

		}
		else
		{
			cout << "Didn't find intersection" << endl;
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
