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

#include "NetworkPackets.h"

#if !defined(SERVER_SIDE)
	#include "ClientApp.h"
#endif

#include <boost/thread/mutex.hpp>

#include <unistd.h>

#define BACKGROUND_LOAD

#define CHUNK_SIZE 64
#define CHUNK_DIST 5
#define NOISE_SCALE 150.0

#define TERRAIN_EXTRACT_TYPE 1

boost::mutex TerrainPager::req_mutex;
std::map<TerrainPager::chunkCoord, bool> TerrainPager::chunkProcessing;

typedef struct ExtractRequestHolder
{
	PolyVox::Region region;
	TerrainPager::chunkCoord coord;
	bool new_mesh;
	PolyVox::SurfaceMesh<PolyVox::PositionMaterial> poly_mesh;
	friend std::ostream& operator<<(std::ostream& os, const struct ExtractRequestHolder &region) { return os; }

} ExtractRequest;

TerrainPager::TerrainPager( Ogre::SceneManager *sceneMgr, Ogre::SceneNode *node, BaseApplication* app ) :
	app(app), volume(&volume_load, &volume_unload, CHUNK_SIZE), manObj(NULL), lastPosition(0,0,0), 
	extractQueue(Ogre::Root::getSingleton().getWorkQueue()), init(false)
{

	volume.setCompressionEnabled(true);
	volume.setMaxNumberOfBlocksInMemory( 1024 );

	volume.prefetch( PolyVox::Region( PolyVox::Vector3DInt32( -CHUNK_DIST*CHUNK_SIZE, 0, -CHUNK_DIST*CHUNK_SIZE ), 
				PolyVox::Vector3DInt32( CHUNK_DIST*CHUNK_SIZE, CHUNK_SIZE-1, CHUNK_DIST*CHUNK_SIZE ) ) );

#ifdef CLIENT_SIDE
	manObj = sceneMgr->createManualObject("terrain");
	node->attachObject(manObj);
#endif

	queueChannel = extractQueue->getChannel("Terrain/Page");

	extractQueue->addRequestHandler( queueChannel, this );
	extractQueue->addResponseHandler( queueChannel, this );

	extractQueue->startup();
}

PolyVox::Material8 TerrainPager::getVoxelAt( const PolyVox::Vector3DInt32 &vec )
{
	return volume.getVoxelAt( vec );
}

void TerrainPager::processUpdate( Packet &packet )
{
	int32_t vecX = packet.pop<int32_t>();
	int32_t vecY = packet.pop<int32_t>();
	int32_t vecZ = packet.pop<int32_t>();

	uint8_t mat = packet.pop<uint8_t>();

	volume.setVoxelAt(PolyVox::Vector3DInt32(vecX, vecY, vecZ), PolyVox::Material8(mat));
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

#ifdef CLIENT_SIDE
	// Notify the server of the change
	Packet modifyPacket;
	modifyPacket.type = TERRAIN_UPDATE;

	modifyPacket.push(vec.getX());
	modifyPacket.push(vec.getY());
	modifyPacket.push(vec.getZ());
	modifyPacket.push(mat.getMaterial());

	modifyPacket.send( ClientApp::getInstance()->getServer(), ENET_PACKET_FLAG_RELIABLE );
#endif
	
	// mark region and neighbors as dirty
	chunkCoord coord = toChunkCoord(vec);

	chunkDirty[ coord ] = true;
	
	if( vec.getX() % CHUNK_SIZE == 0 )
	{
		chunkDirty[ std::make_pair(coord.first-1, coord.second) ] = true;
	}

	if( (vec.getX()+1) % CHUNK_SIZE == 0 )
	{
		chunkDirty[ std::make_pair(coord.first+1, coord.second) ] = true;
	}

	if( vec.getZ() % CHUNK_SIZE == 0 )
	{
		chunkDirty[ std::make_pair(coord.first, coord.second-1) ] = true;
	}

	if( (vec.getZ()+1) % CHUNK_SIZE == 0 )
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

	int beginIndex = req->poly_mesh.m_vecLodRecords[0].beginIndex;
	int endIndex = req->poly_mesh.m_vecLodRecords[0].endIndex;

	if( endIndex != beginIndex )
	{
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
	}

	chunkProcessing[req->coord] = false;
	chunkDirty[req->coord] = false;
	delete req;
}

void TerrainPager::regenerateChunk( chunkCoord coord )
{
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
			std::cout << "Terrain: reload chunk( " << coord.first << ", " << coord.second << ")" << std::endl;
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

// regenerate the mesh for our new position, if needed
void TerrainPager::regenerateMesh( const Ogre::Vector3 &position )
{
	//std::cout << "Terrain: regenMesh( " << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;

	// regen the mesh around our position
	chunkCoord chunk = toChunkCoord( PolyVox::Vector3DInt32( position.x, 0, position.z ) );

	for( int x = chunk.first - CHUNK_DIST; x <= chunk.first + CHUNK_DIST; x++ )
	{
		for( int z = chunk.second - CHUNK_DIST; z <= chunk.second + CHUNK_DIST; z++ )
		{
			regenerateChunk( std::make_pair(x,z) );
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
void TerrainPager::serialize( chunkCoord coord, Packet &packet )
{
	boost::mutex::scoped_lock lock(req_mutex);

	packet.type = TERRAIN_RESPONSE;

	packet.push<int32_t>( coord.first );
	packet.push<int32_t>( coord.second );

	PolyVox::Region region( toRegion( coord ) );

	for( int x = region.getLowerCorner().getX(); x <= region.getUpperCorner().getX(); x++ )
	{
		for( int y = region.getLowerCorner().getY(); y <= region.getUpperCorner().getY(); y++ )
		{
			for( int z = region.getLowerCorner().getZ(); z <= region.getUpperCorner().getZ(); z++ )
			{
				packet.push<uint8_t>(volume.getVoxelAt( PolyVox::Vector3DInt32(x, y, z) ).getMaterial());
			}
		}
	}
}

// extract data from the packet
int TerrainPager::unserialize( Packet &packet )
{
	boost::mutex::scoped_lock lock(req_mutex);

	// undo the above
	// first extract the coord

	int32_t coordX = packet.pop<int32_t>();
	int32_t coordZ = packet.pop<int32_t>();
	chunkCoord coord = std::make_pair( coordX, coordZ );
	PolyVox::Region region( toRegion( coord ) );

	std::cout << "Unserialize chunk: (" << coord.first << ", " << coord.second << ")" << std::endl;

	// now extract the voxels
	for( int x = region.getLowerCorner().getX(); x <= region.getUpperCorner().getX(); x++ )
	{
		for( int y = region.getLowerCorner().getY(); y <= region.getUpperCorner().getY(); y++ )
		{
			for( int z = region.getLowerCorner().getZ(); z <= region.getUpperCorner().getZ(); z++ )
			{
				volume.setVoxelAt( x, y, z, PolyVox::Material8(packet.pop<uint8_t>()) );
			}
		}
	}
	// the chunk has changed
	chunkProcessing[coord] = false;
	chunkProcessing[std::make_pair( coord.first+1, coord.second ) ] = false;
	chunkProcessing[std::make_pair( coord.first-1, coord.second ) ] = false;
	chunkProcessing[std::make_pair( coord.first, coord.second+1 ) ] = false;
	chunkProcessing[std::make_pair( coord.first, coord.second-1 ) ] = false;

	chunkDirty[coord] = true;
	chunkDirty[std::make_pair( coord.first+1, coord.second ) ] = true;
	chunkDirty[std::make_pair( coord.first-1, coord.second ) ] = true;
	chunkDirty[std::make_pair( coord.first, coord.second+1 ) ] = true;
	chunkDirty[std::make_pair( coord.first, coord.second-1 ) ] = true;

	return 0;
}

void TerrainPager::request( Packet &req, Packet &resp )
{
	int32_t coordX = req.pop<int32_t>();
	int32_t coordZ = req.pop<int32_t>();

	chunkCoord coord = std::make_pair( coordX, coordZ );
	std::cout << "Terrain Request received: (" << coord.first << ", " << coord.second << ")" << std::endl;
	serialize( coord, resp );
	std::cout << "Request handled" << std::endl;
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
	chunkCoord coord( toChunkCoord(region.getLowerCorner()) );

	if( region.getLowerCorner().getY() != 0 )
	{
		return;
	}

#ifdef SERVER_SIDE
	std::cout << "Server load chunk: (" << coord.first << ", " << coord.second << ")" << std::endl;
	// generate the new chunk
	// TODO saving/loading
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
#else	// CLIENT_SIDE
	if( chunkProcessing[ coord ] == false )
	{
		chunkProcessing[ coord ] = true;
		std::cout << "Client load chunk: (" << coord.first << ", " << coord.second << ")" << std::endl;
		// ask server for this chunk
		Packet terrainRequest;

		terrainRequest.type = TERRAIN_REQUEST;

		terrainRequest.push<int32_t>(coord.first);
		terrainRequest.push<int32_t>(coord.second);

		// send to server
		terrainRequest.send( ClientApp::getInstance()->getServer(), ENET_PACKET_FLAG_RELIABLE );
	}
#endif
}

void TerrainPager::volume_unload( const PolyVox::ConstVolumeProxy<PolyVox::Material8> &vol, const PolyVox::Region &region )
{
	//std::cout << "Unloading chunk " << region.getLowerCorner() << "->" << region.getUpperCorner() << std::endl;
}

