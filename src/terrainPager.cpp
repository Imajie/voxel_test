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

#include <unistd.h>

#define BACKGROUND_LOAD

#define CHUNK_SIZE 64
#define CHUNK_DIST 5
#define NOISE_SCALE 150.0

#define TERRAIN_EXTRACT_TYPE 1

boost::mutex TerrainPager::req_mutex;

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

PolyVox::Material8 TerrainPager::getVoxelAt( const PolyVox::Vector3DInt32 &vec )
{
	return volume.getVoxelAt( vec );
}

void TerrainPager::setVoxelAt( const PolyVox::Vector3DInt32 &vec, PolyVox::Material8 mat )
{
	boost::mutex::scoped_lock lock(req_mutex);

	if( vec.getY() < 0 || vec.getY() > CHUNK_SIZE-1 )
	{
		std::cout << "setVoxelAt: out of bounds: " << vec << std::endl;
		return;
	}
	volume.setVoxelAt( vec, mat );

	// mark region and neighbors as dirty
	chunkCoord coord = toChunkCoord(vec);

	chunkDirty[ coord ] = true;
	
	if( vec.getX() % CHUNK_SIZE == 0 )
	{
		chunkDirty[ std::make_pair(coord.first-1, coord.second) ] = true;
	}

	if( vec.getX()+1 % CHUNK_SIZE == 0 )
	{
		chunkDirty[ std::make_pair(coord.first+1, coord.second) ] = true;
	}

	if( vec.getZ() % CHUNK_SIZE == 0 )
	{
		chunkDirty[ std::make_pair(coord.first, coord.second-1) ] = true;
	}

	if( vec.getZ()+1 % CHUNK_SIZE == 0 )
	{
		chunkDirty[ std::make_pair(coord.first, coord.second+1) ] = true;
	}
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
	ExtractRequest *data = req->getData().get<ExtractRequest*>();

	extract( data->region, data->poly_mesh );

	usleep(1000);
	
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
	boost::mutex::scoped_lock lock(resp_mutex);
	ExtractRequest *req = res->getRequest()->getData().get<ExtractRequest*>();

	if( req->new_mesh )
	{
		// add chunk to map
		chunkToMesh[req->coord] = manObj->getNumSections();

		// add chunk to map
		manObj->begin("VoxelTexture", Ogre::RenderOperation::OT_TRIANGLE_LIST);
	}
	else
	{
		manObj->beginUpdate( chunkToMesh[req->coord] );
	}

	genMesh( req->region, req->poly_mesh );

	manObj->end();

	chunkProcessing[req->coord] = false;
	chunkDirty[req->coord] = false;
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
				genMesh( req->region, req->poly_mesh );
				
				manObj->end();

				// add chunk to map
				chunkToMesh[req->coord] = manObj->getNumSections();
#else
				if( chunkProcessing[ coord ] == false )
				{
					chunkProcessing[ coord ] = true;
					extractQueue->addRequest(queueChannel, TERRAIN_EXTRACT_TYPE, Ogre::Any(req));
				}
#endif
			}
			else
			{
				if( chunkDirty[coord] == true )
				{
#ifndef BACKGROUND_LOAD
					manObj->beginUpdate( chunkToMesh[ coord ] );

					extract( req->region, req->poly_mesh );
					genMesh( req->region, req->poly_mesh );
					
					manObj->end();

#else
					if( chunkProcessing[ coord ] == false )
					{
						chunkProcessing[ coord ] = true;
						extractQueue->addRequest(queueChannel, TERRAIN_EXTRACT_TYPE, Ogre::Any(req));
					}
#endif
					chunkDirty[coord] = false;
				}
			}
		}
	}

	lastPosition = position;
}

bool raycastIsPassable( const PolyVox::LargeVolume<PolyVox::Material8>::Sampler &sampler )
{
	if( sampler.getVoxel().getMaterial() == 0 )
		return true;
	return false;
}

void TerrainPager::raycast( const PolyVox::Vector3DFloat &start, const PolyVox::Vector3DFloat &dir, PolyVox::RaycastResult &result )
{
	boost::mutex::scoped_lock lock(req_mutex);
	
	PolyVox::Raycast< PolyVox::LargeVolume<PolyVox::Material8> > caster(&volume, start, dir, result, raycastIsPassable);

	caster.execute();
}

void TerrainPager::extract( const PolyVox::Region &region, PolyVox::SurfaceMesh<PolyVox::PositionMaterial> &surf_mesh )
{
	boost::mutex::scoped_lock lock(req_mutex);

	volume.prefetch( region );

	PolyVox::CubicSurfaceExtractor<PolyVox::LargeVolume<PolyVox::Material8> > suf(&volume, region, &surf_mesh, false);

	suf.execute();
}

void TerrainPager::genMesh( const PolyVox::Region &region, const PolyVox::SurfaceMesh<PolyVox::PositionMaterial> &surf_mesh )
{
	const std::vector<PolyVox::PositionMaterial>& vecVertices = surf_mesh.getVertices();
	const std::vector<uint32_t>& vecIndices = surf_mesh.getIndices();

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

		uint8_t mat = vertex.getMaterial() - 1;
		
		manObj->colour(mat, mat, mat);
	}
}

// TODO: for now just send 1 byte per voxel

// serialize for a chunk
ENetPacket* TerrainPager::serialize( chunkCoord coord )
{
	boost::mutex::scoped_lock lock(req_mutex);

	// allocate enough buffer to hold each voxel plus the lower/upper corner position
	int buffer_size = CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE + 2*sizeof(int32_t);

	char *buffer = new char[buffer_size];

	// set chunk coordinates
	((int32_t*)buffer)[0] = coord.first;
	((int32_t*)buffer)[1] = coord.second;

	int buffer_offset = 2*sizeof(int32_t);

	PolyVox::Region region( toRegion( coord ) );

	for( int x = region.getLowerCorner().getX(); x < region.getUpperCorner().getX(); x++ )
	{
		for( int y = region.getLowerCorner().getY(); y < region.getUpperCorner().getY(); y++ )
		{
			for( int z = region.getLowerCorner().getZ(); z < region.getUpperCorner().getZ(); z++ )
			{
				buffer[buffer_offset++] = getVoxelAt( PolyVox::Vector3DInt32(x, y, z) ).getMaterial();
			}
		}
	}

	ENetPacket *packet = enet_packet_create( buffer, buffer_size, ENET_PACKET_FLAG_RELIABLE );

	delete [] buffer;
	return packet;
}

// extract data from the packet
int TerrainPager::unserialize( ENetPacket *packet )
{
	boost::mutex::scoped_lock lock(req_mutex);

	// undo the above
	// first extract the coord
	int32_t *coord_data = (int32_t*)packet->data;
	
	chunkCoord coord = std::make_pair( coord_data[0], coord_data[1] );
	PolyVox::Region region( toRegion( coord ) );

	uint8_t *buffer = packet->data;
	int buffer_offset = 2*sizeof(int32_t);

	// now extract the voxels
	for( int x = region.getLowerCorner().getX(); x < region.getUpperCorner().getX(); x++ )
	{
		for( int y = region.getLowerCorner().getY(); y < region.getUpperCorner().getY(); y++ )
		{
			for( int z = region.getLowerCorner().getZ(); z < region.getUpperCorner().getZ(); z++ )
			{
				volume.setVoxelAt( x, y, z, PolyVox::Material8(buffer[buffer_offset++]) );
			}
		}
	}
	// the chunk has changed
	chunkDirty[coord] = true;

	return 0;
}

const PolyVox::Region TerrainPager::toRegion( const chunkCoord &coord )
{
	int x = coord.first  * CHUNK_SIZE;
	int z = coord.second * CHUNK_SIZE;

	PolyVox::Region region;

	region.setLowerCorner( PolyVox::Vector3DInt32( x, 0, z ) );
	region.setUpperCorner( PolyVox::Vector3DInt32( x + CHUNK_SIZE-1, CHUNK_SIZE-1, z + CHUNK_SIZE-1 ) );

	return region;
}

TerrainPager::chunkCoord TerrainPager::toChunkCoord( const PolyVox::Vector3DInt32 &vec )
{
	int x = floor(((double)vec.getX()) / CHUNK_SIZE);
	int z = floor(((double)vec.getZ()) / CHUNK_SIZE);

	return std::make_pair(x,z);
}

// volume paging functions
void TerrainPager::volume_load( const PolyVox::ConstVolumeProxy<PolyVox::Material8> &vol, const PolyVox::Region &region )
{
	if( region.getLowerCorner().getY() != 0 )
	{
		return;
	}

	for( int x = region.getLowerCorner().getX(); x <= region.getUpperCorner().getX(); x++ )
	{
		for( int z = region.getLowerCorner().getZ(); z <= region.getUpperCorner().getZ(); z++ )
		{
			double height = (CHUNK_SIZE*perlinNoise(x/NOISE_SCALE, z/NOISE_SCALE)/2.0 + CHUNK_SIZE/2.0);

			for( int y = 0; y < std::min(height, CHUNK_SIZE - 1.0); y++ )
			{

				PolyVox::Material8 voxel = vol.getVoxelAt(x, y, z);

				if( y < CHUNK_SIZE/3 )
				{
					voxel.setMaterial(1);
				}
				else if( y < 2*CHUNK_SIZE/3 )
				{
					voxel.setMaterial(2);
				}
				else if( y < 3*CHUNK_SIZE/3 )
				{
					voxel.setMaterial(3);
				}
				vol.setVoxelAt(x,y,z, voxel);
			}
		}
	}
}

void TerrainPager::volume_unload( const PolyVox::ConstVolumeProxy<PolyVox::Material8> &vol, const PolyVox::Region &region )
{
	//std::cout << "Unloading chunk " << region.getLowerCorner() << "->" << region.getUpperCorner() << std::endl;
}

