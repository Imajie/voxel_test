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

#define CHUNK_SIZE 64
#define CHUNK_DIST 5
#define NOISE_SCALE 150.0

TerrainPager::TerrainPager( Ogre::SceneManager *sceneMgr, Ogre::SceneNode *node ) :
	volume(&volume_load, &volume_unload, CHUNK_SIZE), mesh(sceneMgr->createManualObject("terrain")), lastPosition(0,0,0), init(false)
{
	mesh->setDynamic(true);
	node->attachObject(mesh);
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

			if( chunkToMesh.find( coord ) == chunkToMesh.end() )
			{
				volume.prefetch( region );

				// add chunk to map
				chunkToMesh[toChunkCoord(region.getLowerCorner())] = mesh->getNumSections();

				mesh->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);
				extract( region );
				mesh->end();

			}
			else
			{
				if( false )
				{
					mesh->beginUpdate( chunkToMesh[coord] );
					extract( region );
					mesh->end();
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

void TerrainPager::extract( const PolyVox::Region &region )
{
	PolyVox::SurfaceMesh<PolyVox::PositionMaterial> surf_mesh;

	PolyVox::CubicSurfaceExtractor<PolyVox::LargeVolume, PolyVox::Material8> suf(&volume, region, &surf_mesh);

	suf.execute();

	const std::vector<PolyVox::PositionMaterial>& vecVertices = surf_mesh.getVertices();
	const std::vector<uint32_t>& vecIndices = surf_mesh.getIndices();

	unsigned int uLodLevel = 0;
	int beginIndex = surf_mesh.m_vecLodRecords[uLodLevel].beginIndex;
	int endIndex = surf_mesh.m_vecLodRecords[uLodLevel].endIndex;

	if( endIndex == beginIndex )
	{
		std::cout << "Index count = 0" << std::endl;
	}

	for(int index = beginIndex; index < endIndex; ++index) {
		const PolyVox::PositionMaterial& vertex = vecVertices[vecIndices[index]];

		const PolyVox::Vector3DFloat& v3dVertexPos = vertex.getPosition();

		const PolyVox::Vector3DFloat v3dFinalVertexPos = v3dVertexPos + static_cast<PolyVox::Vector3DFloat>(surf_mesh.m_Region.getLowerCorner());

		mesh->position(v3dFinalVertexPos.getX(), v3dFinalVertexPos.getY(), v3dFinalVertexPos.getZ());

		uint8_t red = ((int)abs(vertex.getPosition().getY())) % 256;
		uint8_t green = ((int)abs(vertex.getPosition().getZ())*3) % 256;
		uint8_t blue = ((int)abs(vertex.getPosition().getX())*5) % 256;

		mesh->colour(red, green, blue);
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
	for( int x = region.getLowerCorner().getX(); x < region.getUpperCorner().getX(); x++ )
	{
		for( int z = region.getLowerCorner().getZ(); z < region.getUpperCorner().getZ(); z++ )
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
	//std::cout << "Unloading chunk " << region.getLowerCorner() << "->" << region.getUpperCorner() << std::endl;
}

