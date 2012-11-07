/*
 * File:	terrainGenerator.h
 * Author:	James Letendre
 *
 * Generate heightmap using diamond square
 */
#ifndef TERRAIN_GENERATOR_H
#define TERRAIN_GENERATOR_H

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cmath>

class TerrainGenerator
{
	public:
		TerrainGenerator( uint32_t size, float scaleFact );
		
		// get width/height of map
		uint32_t width() const { return _size; }
		uint32_t height() const { return _size; }

		// get value of height map at position
		float get( double x, double y ) const;

//		vector3df getNormal(uint32_t x, uint32_t y, float s) const;

		//float *getHeightMap() { return heightMap; }

	private:
		// set value in height map
		void set( uint32_t x, uint32_t y, float val ) 
		{
			int idx = (y%_size)*_size + (x%_size);
			heightMap[idx] = val; 
		}

		// run diamondSquare
		void diamondSquare();

		uint32_t _size;
		float _scaleFact;
		float* heightMap;

};

#endif
