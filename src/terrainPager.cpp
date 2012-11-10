/*
 * File:	terrainPager.cpp
 * Author:	James Letendre
 *
 * Page in/out terrain chunks
 */
#include "terrainPager.h"
#include "perlinNoise.h"

#include <PolyVoxCore/CubicSurfaceExtractor.h>
#include <vector>
#include <OgreRoot.h>
#include <OgreMeshManager.h>
#include <OgreEntity.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>

#include <boost/thread/mutex.hpp>

#define BACKGROUND_LOAD

#define CHUNK_SIZE 64
#define CHUNK_DIST 5
#define NOISE_SCALE 150.0

#define TERRAIN_EXTRACT_TYPE 1

typedef struct ExtractRequestHolder
{
	PolyVox::Region region;
	TerrainPager::chunkCoord coord;
	bool new_mesh;
	PolyVox::SurfaceMesh<PolyVox::PositionMaterial> poly_mesh;
	friend std::ostream& operator<<(std::ostream& os, const struct ExtractRequestHolder &region) { return os; }

} ExtractRequest;

TerrainPager::TerrainPager( Ogre::SceneManager *sceneMgr, Ogre::SceneNode *node ) :
	volume(&volume_load, &volume_unload, CHUNK_SIZE), manObj(sceneMgr->createManualObject("terrain")), lastPosition(0,0,0), 
	extractQueue(Ogre::Root::getSingleton().getWorkQueue()), init(false)
{
	volume.setCompressionEnabled(true);
	volume.setMaxNumberOfBlocksInMemory( 1024 );

	node->attachObject(manObj);

	queueChannel = extractQueue->getChannel("Terrain/Page");

	extractQueue->addRequestHandler( queueChannel, this );
	extractQueue->addResponseHandler( queueChannel, this );

	extractQueue->startup();
}

bool TerrainPager::canHandleRequest (const Ogre::WorkQueue::Request *req, const Ogre::WorkQueue *srcQ)
{
	if( req->getType() == TERRAIN_EXTRACT_TYPE )
	{
		return true;
	}
	return false;
}

Ogre::WorkQueue::Response* TerrainPager::handleRequest (const Ogre::WorkQueue::Request *req, const Ogre::WorkQueue *srcQ)
{
	// lock
	boost::unique_lock<boost::mutex> lock(req_mutex);
	std::cout << "Request" << std::endl;

	ExtractRequest *data = req->getData().get<ExtractRequest*>();

	extract( data->region, data->poly_mesh );
	
	//std::cout << "Request" << std::endl;
	//std::cout << "vertex count = " << data->poly_mesh.getVertices().size() << std::endl;
	//std::cout << "vertex[0] = " << data->poly_mesh.getVertices()[100].getPosition() << std::endl;

	std::cout << "Done" << std::endl;
	return new Ogre::WorkQueue::Response( req, true, req->getData() );
}

bool TerrainPager::canHandleResponse (const Ogre::WorkQueue::Response *res, const Ogre::WorkQueue *srcQ)
{
	if( res->getRequest()->getType() == TERRAIN_EXTRACT_TYPE )
	{
		return true;
	}
	return false;
}

void TerrainPager::handleResponse (const Ogre::WorkQueue::Response *res, const Ogre::WorkQueue *srcQ)
{
	// lock
	boost::unique_lock<boost::mutex> lock(resp_mutex);
	ExtractRequest *req = res->getRequest()->getData().get<ExtractRequest*>();

	if( req->new_mesh )
	{
		// add chunk to map
		manObj->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);
	}
	else
	{
		manObj->beginUpdate( chunkToMesh[req->coord] );
	}

	genMesh( req->region, req->poly_mesh, req->new_mesh );

	manObj->end();

	// add chunk to map
	chunkToMesh[req->coord] = manObj->getNumSections();

	chunkProcessing[req->coord] = false;
	delete req;
}

// regenerate the mesh for our new position, if needed
void TerrainPager::regenerateMesh( const Ogre::Vector3 &position )
{
	// regen the mesh around our position
	chunkCoord chunk = toChunkCoord( PolyVox::Vector3DInt32( position.x, 0, position.z ) );

	for( int x = chunk.first - CHUNK_DIST; x <= chunk.first + CHUNK_DIST; x++ )
	{
		for( int z = chunk.second - CHUNK_DIST; z <= chunk.second + CHUNK_DIST; z++ )
		{
			chunkCoord coord = std::make_pair(x,z);
			PolyVox::Region region = toRegion(coord);

			ExtractRequest *req = new ExtractRequest;
			req->region = region;
			req->coord = coord;
			req->new_mesh = false;

			if( chunkToMesh.find( coord ) == chunkToMesh.end() )
			{
				req->new_mesh = true;

#ifndef BACKGROUND_LOAD
				manObj->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);

				extract( req->region, req->poly_mesh );
				genMesh( req->region, req->poly_mesh, req->new_mesh );
				
				manObj->end();

				// add chunk to map
				chunkToMesh[req->coord] = manObj->getNumSections();
#else
				if( chunkProcessing.find( coord ) == chunkProcessing.end() )
				{
					chunkProcessing[ coord ] = true;
					extractQueue->addRequest(queueChannel, TERRAIN_EXTRACT_TYPE, Ogre::Any(req));
				}
#endif
			}
			else
			{
				if( false )
				{
					if( chunkProcessing.find( coord ) == chunkProcessing.end() )
					{
						chunkProcessing[ coord ] = true;
						extractQueue->addRequest(queueChannel, TERRAIN_EXTRACT_TYPE, Ogre::Any(req));
					}
				}
			}
		}
	}

	lastPosition = position;
}

void TerrainPager::raycast( const PolyVox::Vector3DFloat &start, const PolyVox::Vector3DFloat &dir, PolyVox::RaycastResult &result )
{
	PolyVox::Raycast<PolyVox::LargeVolume, PolyVox::Material8> caster(&volume, start, dir, result);

	caster.execute();
}

void TerrainPager::extract( const PolyVox::Region &region, PolyVox::SurfaceMesh<PolyVox::PositionMaterial> &surf_mesh )
{
	PolyVox::Region new_region;

	new_region.setUpperCorner( region.getUpperCorner() - PolyVox::Vector3DInt32(1, 0, 1) );
	new_region.setLowerCorner( region.getLowerCorner() );

	volume.prefetch( region );
	PolyVox::CubicSurfaceExtractor<PolyVox::LargeVolume, PolyVox::Material8> suf(&volume, new_region, &surf_mesh);

	suf.execute();
}

void TerrainPager::genMesh( const PolyVox::Region &region, const PolyVox::SurfaceMesh<PolyVox::PositionMaterial> &surf_mesh, bool new_mesh )
{
	const std::vector<PolyVox::PositionMaterial>& vecVertices = surf_mesh.getVertices();
	const std::vector<uint32_t>& vecIndices = surf_mesh.getIndices();

	//std::cout << "Response" << std::endl;
	//std::cout << "vertex count = " << vecVertices.size() << std::endl;
	//std::cout << "vertex[0] = " << vecVertices[100].getPosition() << std::endl;

	unsigned int uLodLevel = 0;
	int beginIndex = surf_mesh.m_vecLodRecords[uLodLevel].beginIndex;
	int endIndex = surf_mesh.m_vecLodRecords[uLodLevel].endIndex;

	if( endIndex == beginIndex )
	{
		std::cout << "Index count = " << endIndex-beginIndex << std::endl;
	}

	for(int index = beginIndex; index < endIndex; ++index) {
		const PolyVox::PositionMaterial& vertex = vecVertices[vecIndices[index]];

		const PolyVox::Vector3DFloat& v3dVertexPos = vertex.getPosition();

		const PolyVox::Vector3DFloat v3dFinalVertexPos = v3dVertexPos + static_cast<PolyVox::Vector3DFloat>(surf_mesh.m_Region.getLowerCorner());

		manObj->position(v3dFinalVertexPos.getX(), v3dFinalVertexPos.getY(), v3dFinalVertexPos.getZ());

		uint8_t red = ((int)abs(vertex.getPosition().getY())) % 256;
		uint8_t green = ((int)abs(vertex.getPosition().getZ())*3) % 256;
		uint8_t blue = ((int)abs(vertex.getPosition().getX())*5) % 256;

		manObj->colour(red, green, blue);
	}
}

const PolyVox::Region TerrainPager::toRegion( const chunkCoord &coord )
{
	int x = coord.first  * CHUNK_SIZE;
	int z = coord.second * CHUNK_SIZE;

	PolyVox::Region region;

	region.setLowerCorner( PolyVox::Vector3DInt32( x, 0, z ) );
	region.setUpperCorner( PolyVox::Vector3DInt32( x + CHUNK_SIZE, CHUNK_SIZE-1, z + CHUNK_SIZE ) );

	return region;
}

TerrainPager::chunkCoord TerrainPager::toChunkCoord( const PolyVox::Vector3DInt32 &vec )
{
	int x = vec.getX() / CHUNK_SIZE;
	int z = vec.getZ() / CHUNK_SIZE;

	return std::make_pair(x,z);
}

// volume paging functions
void TerrainPager::volume_load( const PolyVox::ConstVolumeProxy<PolyVox::Material8> &vol, const PolyVox::Region &region )
{
	boost::unique_lock<boost::mutex>(req_mutex);
	for( int x = region.getLowerCorner().getX(); x <= region.getUpperCorner().getX(); x++ )
	{
		for( int z = region.getLowerCorner().getZ(); z <= region.getUpperCorner().getZ(); z++ )
		{
			double height = (CHUNK_SIZE*perlinNoise(x/NOISE_SCALE, z/NOISE_SCALE)/2.0 + CHUNK_SIZE/2.0);

			for( int y = 0; y < std::min(height, CHUNK_SIZE - 1.0); y++ )
			{
				PolyVox::Material8 voxel = vol.getVoxelAt(x, y, z);
				voxel.setMaterial(1);
				vol.setVoxelAt(x,y,z, voxel);
			}
		}
	}
}

void TerrainPager::volume_unload( const PolyVox::ConstVolumeProxy<PolyVox::Material8> &vol, const PolyVox::Region &region )
{
	boost::unique_lock<boost::mutex>(req_mutex);
	//std::cout << "Unloading chunk " << region.getLowerCorner() << "->" << region.getUpperCorner() << std::endl;
}

