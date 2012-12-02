/*
 * File:	terrainPager.h
 * Author:	James Letendre
 *
 * Page in/out terrain chunks
 */
#ifndef TERRAIN_PAGER_H
#define TERRAIN_PAGER_H

#include <map>

#include "BaseApplication.h"

#include <PolyVoxCore/LargeVolume.h>
#include <PolyVoxCore/SimpleInterface.h>
#include <PolyVoxCore/Material.h>
#include <PolyVoxCore/Raycast.h>

#include <OgreSceneManager.h>
#include <OgreManualObject.h>
#include <OgreWorkQueue.h>

#include "NetworkPackets.h"

class TerrainPager : public Ogre::WorkQueue::RequestHandler, public Ogre::WorkQueue::ResponseHandler
{
	public:
		typedef std::pair<int,int> chunkCoord;
		
		TerrainPager( Ogre::SceneManager *sceneMgr, Ogre::SceneNode *node, BaseApplication *app );

		// regenerate the mesh for our new position, if needed
		void regenerateChunk( chunkCoord coord );
		void regenerateMesh( const Ogre::Vector3 &position );

		// raycast into the volume
		void raycast( const PolyVox::Vector3DFloat &start, const PolyVox::Vector3DFloat &dir, PolyVox::RaycastResult &result );

		static void volume_load( const PolyVox::ConstVolumeProxy<PolyVox::Material8> &vol, const PolyVox::Region &region );
		static void volume_unload( const PolyVox::ConstVolumeProxy<PolyVox::Material8> &vol, const PolyVox::Region &region );

		// serialization
		void serialize( chunkCoord coord, Packet &packet );
		int unserialize( Packet &packet );
		void request( Packet &req, Packet &resp );

		// volume interface
		PolyVox::Region getEnclosingRegion() { return volume.getEnclosingRegion(); }
		PolyVox::Material8 getVoxelAt( const PolyVox::Vector3DInt32 &vec );
		void setVoxelAt( const PolyVox::Vector3DInt32 &vec, PolyVox::Material8 mat );
		void processUpdate( Packet &packet );

		// lock
		void lock() { mutex.lock(); }
		void unlock() { mutex.unlock(); }

		// convert the region into the chunk coordinates
		static chunkCoord toChunkCoord( const PolyVox::Vector3DInt32 &vec );

		// convert chunk coordinates into region
		static const PolyVox::Region toRegion( const chunkCoord &coord );

		const void dumpDebug()
		{
			printf("chunkToMesh:\n");
			for( auto m : chunkToMesh )
			{
				printf("\t(%i, %i)->%i\n", m.first.first, m.first.second, m.second);
			}
			printf("\n");
		}

	private:
		BaseApplication *app;

		// Request handler interface
		virtual bool canHandleRequest (const Ogre::WorkQueue::Request *req, const Ogre::WorkQueue *srcQ);
		virtual Ogre::WorkQueue::Response* handleRequest (const Ogre::WorkQueue::Request *req, const Ogre::WorkQueue *srcQ);

		// Response handler interface
		virtual bool canHandleResponse (const Ogre::WorkQueue::Response *res, const Ogre::WorkQueue *srcQ);
		virtual void handleResponse (const Ogre::WorkQueue::Response *res, const Ogre::WorkQueue *srcQ);

		// extract the region into new/updated mesh
		void extract( const PolyVox::Region &region, PolyVox::SurfaceMesh<PolyVox::PositionMaterial> &surf_mesh );
		void genMesh( const PolyVox::Region &region, const PolyVox::SurfaceMesh<PolyVox::PositionMaterial> &surf_mesh );

		// the volume to page
		PolyVox::LargeVolume<PolyVox::Material8> volume;

		// the ogre mesh
		Ogre::ManualObject *manObj;

		// last player position
		Ogre::Vector3 lastPosition;

		// Work queue for loading terrain in the background
		Ogre::WorkQueue *extractQueue;
		int queueChannel;
		static boost::mutex req_mutex;
		boost::mutex resp_mutex;
		boost::mutex mutex;

		// initialized
		bool init;

		// mapping from chunk coord to mesh id
		std::map<chunkCoord, int> chunkToMesh;
		static std::map<chunkCoord, bool> chunkProcessing;
		std::map<chunkCoord, bool> chunkDirty;
};

#endif

