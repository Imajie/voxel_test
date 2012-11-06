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

#include "BasicTutorial2.h"
#include "terrainGenerator.h"
#include "PolyVoxCore/SimpleVolume.h"
#include "PolyVoxCore/SimpleInterface.h"
#include "PolyVoxCore/CubicSurfaceExtractor.h"
#include "PolyVoxCore/SurfaceExtractor.h"
#include "PolyVoxCore/MaterialDensityPair.h"
#include "PolyVoxCore/SurfaceMesh.h"

#define TERRAIN_SIZE ((1<<10)+1)

typedef PolyVox::MaterialDensityPair<uint8_t, 7, 1> MyMaterialDensityPair;

using namespace std;

void putHeightMapInVolume(PolyVox::SimpleVolume<MyMaterialDensityPair> &volume, TerrainGenerator &terrainGen)
{
	PolyVox::Region volRegion(volume.getEnclosingRegion());

	for (int z = volRegion.getLowerCorner().getZ(); z < volRegion.getUpperCorner().getZ(); z++)
	{
		for (int x = volRegion.getLowerCorner().getX(); x < volRegion.getUpperCorner().getX(); x++)
		{
			if( x - volRegion.getLowerCorner().getX() < (int)terrainGen.width() &&
				z - volRegion.getLowerCorner().getZ() < (int)terrainGen.height() )
			{
				for( int y = volRegion.getLowerCorner().getY(); y <
						terrainGen.get(x - volRegion.getLowerCorner().getX(), z - volRegion.getLowerCorner().getZ()); y++ )
				{
					//Our new density value
					uint8_t uDensity = MyMaterialDensityPair::getMaxDensity();

					//Get the old voxel
					MyMaterialDensityPair voxel = volume.getVoxelAt(x,y,z);

					//Modify the density
					voxel.setDensity(uDensity);

					//Wrte the voxel value into the volume
					volume.setVoxelAt(x, y, z, voxel);
				}
			}
		}
	}

}

void createSphereInVolume(PolyVox::SimpleVolume<MyMaterialDensityPair>& volData, float fRadius, PolyVox::Vector3DFloat center)
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
					uint8_t uDensity = MyMaterialDensityPair::getMaxDensity();

					//Get the old voxel
					MyMaterialDensityPair voxel = volData.getVoxelAt(x,y,z);

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
:   mTerrainGlobals(0),
	mTerrainGroup(0),
	mTerrainsImported(false),
	mInfoLabel(0)
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
	Ogre::ManualObject* ogreMesh;
	// create something to draw the PolyVox stuff to
	ogreMesh = mSceneMgr->createManualObject("PolyVox Mesh");
	// YES we do intend to change the mesh later -.-
	ogreMesh->setDynamic(true);

	Ogre::SceneNode* ogreNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("testnode1", Ogre::Vector3(20, 0, 0));
	ogreNode->attachObject(ogreMesh);
	ogreNode->setScale( Ogre::Vector3(100, 100, 100) );

	PolyVox::SimpleVolume<MyMaterialDensityPair> volume(
			PolyVox::Region(
				PolyVox::Vector3DInt32(-64, -64, -64),
				PolyVox::Vector3DInt32(64, 64, 64)
				)
			);

	// now add some data to it
	TerrainGenerator terrainGen( 128, 300 );

	putHeightMapInVolume(volume, terrainGen);

	//createSphereInVolume(volume, 100.0, PolyVox::Vector3DFloat(0, 0, 0));

	//PolyVox::SurfaceMesh<PolyVox::PositionMaterialNormal> mesh;
	PolyVox::SurfaceMesh<PolyVox::PositionMaterial> mesh;

	std::cout << "updating volume surface" << std::endl;
	//PolyVox::MarchingCubesSurfaceExtractor<PolyVox::SimpleVolume<MyMaterialDensityPair> > suf(&volume, volume.getEnclosingRegion(), &mesh);
	PolyVox::CubicSurfaceExtractor<PolyVox::SimpleVolume, MyMaterialDensityPair > suf(&volume, volume.getEnclosingRegion(), &mesh);
	suf.execute();

	std::cout << "drawing mesh: " << mesh.getNoOfVertices() << std::endl;

	ogreMesh->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);
	{
		//const vector<PolyVox::PositionMaterialNormal>& vecVertices = mesh.getVertices();
		const vector<PolyVox::PositionMaterial>& vecVertices = mesh.getVertices();
		const vector<uint32_t>& vecIndices = mesh.getIndices();
		unsigned int uLodLevel = 0;
		int beginIndex = mesh.m_vecLodRecords[uLodLevel].beginIndex;
		int endIndex = mesh.m_vecLodRecords[uLodLevel].endIndex;
		for(int index = beginIndex; index < endIndex; ++index) {
			//const PolyVox::PositionMaterialNormal& vertex = vecVertices[vecIndices[index]];
			const PolyVox::PositionMaterial& vertex = vecVertices[vecIndices[index]];

			const PolyVox::Vector3DFloat& v3dVertexPos = vertex.getPosition();
			//const PolyVox::Vector3DFloat& v3dVertexNormal = vertex.getNormal();

			const PolyVox::Vector3DFloat v3dFinalVertexPos = v3dVertexPos + static_cast<PolyVox::Vector3DFloat>(mesh.m_Region.getLowerCorner());

			ogreMesh->position(v3dFinalVertexPos.getX(), v3dFinalVertexPos.getY(), v3dFinalVertexPos.getZ());
			//ogreMesh->normal(v3dVertexNormal.getX(), v3dVertexNormal.getY(), v3dVertexNormal.getZ());

			uint8_t red = ((int)(vertex.getPosition().getX()*vertex.getPosition().getZ()+vertex.getPosition().getY()) % 256);
			uint8_t green = ((int)(vertex.getPosition().getY()*vertex.getPosition().getX()) % 256);
			uint8_t blue = ((int)(vertex.getPosition().getZ()*vertex.getPosition().getY()*vertex.getPosition().getX()) % 256);

			ogreMesh->colour(red, green, blue);// just some random colors, I'm too lazy for hsv
		}
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

	mInfoLabel = mTrayMgr->createLabel(OgreBites::TL_TOP, "TInfo", "", 350);
}
//-------------------------------------------------------------------------------------
bool BasicTutorial3::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	bool ret = BaseApplication::frameRenderingQueued(evt);

	return ret;
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
