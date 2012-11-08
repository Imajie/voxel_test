/*
 * File:	terrainPager.h
 * Author:	James Letendre
 *
 * Page in/out terrain chunks
 */
#ifndef TERRAIN_PAGER_H
#define TERRAIN_PAGER_H

#include <map>

#include <PolyVoxCore/LargeVolume.h>
#include <PolyVoxCore/SimpleInterface.h>
#include <PolyVoxCore/Material.h>
#include <PolyVoxCore/Raycast.h>

#include <OgreSceneManager.h>
#include <OgreManualObject.h>

class TerrainPager
{
	public:
		TerrainPager( Ogre::SceneManager *sceneMgr, Ogre::SceneNode *node );

		// regenerate the mesh for our new position, if needed
		void regenerateMesh( const Ogre::Vector3 &position );

		// raycast into the volume
		void raycast( const PolyVox::Vector3DFloat &start, const PolyVox::Vector3DFloat &dir, PolyVox::RaycastResult &result );

		static void volume_load( const PolyVox::ConstVolumeProxy<PolyVox::Material8> &vol, const PolyVox::Region &region );
		static void volume_unload( const PolyVox::ConstVolumeProxy<PolyVox::Material8> &vol, const PolyVox::Region &region );

	private:
		typedef std::pair<int,int> chunkCoord;

		// extract the region into new/updated mesh
		void extract( const PolyVox::Region &region );

		// convert the region into the chunk coordinates
		chunkCoord toChunkCoord( const PolyVox::Vector3DInt32 &vec );

		// convert chunk coordinates into region
		const PolyVox::Region toRegion( const chunkCoord &coord );

		// the volume to page
		PolyVox::LargeVolume<PolyVox::Material8> volume;

		// the ogre mesh
		Ogre::ManualObject *mesh;

		// last player position
		Ogre::Vector3 lastPosition;

		// initialized
		bool init;

		// mapping from chunk coord to mesh id
		std::map<chunkCoord, int> chunkToMesh;
};

#endif

