/*
 * File:	terrainGenerator.cpp
 * Author:	James Letendre
 *
 * Generate heightmap using diamond square
 */
#include "terrainGenerator.h"

#include <iostream>
#include <cstdlib>
#include <time.h>
#include "perlinNoise.h"

using namespace std;

// smaller = steeper more frequent hills
#define TERRAIN_SCALE 150.0

TerrainGenerator::TerrainGenerator( uint32_t size, float scaleFact ) :
	_size(size), _scaleFact(scaleFact), heightMap(new float[size*size])
{
	//diamondSquare();
	initPerlinNoise();
}

float TerrainGenerator::get( double x, double y ) const 
{ 
	return _scaleFact*perlinNoise(x/TERRAIN_SCALE, y/TERRAIN_SCALE);
	//return heightMap[(y%_size)*_size + (x%_size)]; 
}

void TerrainGenerator::diamondSquare()
{
	// seed random number generator
	srand(time(NULL));

	// clear array
	for( uint32_t i = 0; i < _size*_size; i++ )
	{
		heightMap[i] = 0.0;
	}

	float randMag = _scaleFact;
	for( int sideLength = _size-1; sideLength >= 2; sideLength /= 2, randMag /= 2.0 )
	{
		int halfSide = sideLength / 2;

		// Diamond step for each square
		for( uint32_t x = 0; x < _size-1; x+=sideLength )
		{
			for( uint32_t y = 0; y < _size-1; y+=sideLength )
			{
				double avg = 
					get(x,				y) + 
					get(x+sideLength,	y) +
					get(x,				y+sideLength) +
					get(x+sideLength,	y+sideLength);

				avg /= 4.0;

				double offset = ((2.0*rand()/RAND_MAX) - 1) * randMag;

				// middle value is avg + random offset
				set(x+halfSide, y+halfSide, avg+offset);
			}
		}

		// Square step for each diamond
		for (uint32_t x=halfSide; x<_size-1; x+=halfSide) 
		{
			for (uint32_t y=(x+halfSide)%sideLength; y<_size-1; y+=sideLength) 
			{
				double offset = ((2.0*rand()/RAND_MAX) - 1) * randMag;

				float a = get(x - halfSide, y);
				float b = get(x, y - halfSide);
				float c = get(x + halfSide, y);
				float d = get(x, y + halfSide);
				//float e = get(x - sideLength / 2, y - sideLength / 2);

				set(x, y, (a + b + c + d) / 4.0 + offset);
			}
		}
	}
}

/*
vector3df TerrainGenerator::getNormal(uint32_t x, uint32_t y, float s) const
{
	const float zc = get(x, y);
	float zl, zr, zu, zd;

	if (x == 0)
	{
		zr = get(x + 1, y);
		zl = zc + zc - zr;
	}
	else if (x == _size - 1)
	{
		zl = get(x - 1, y);
		zr = zc + zc - zl;
	}
	else
	{
		zr = get(x + 1, y);
		zl = get(x - 1, y);
	}

	if (y == 0)
	{
		zd = get(x, y + 1);
		zu = zc + zc - zd;
	}
	else if (y == _size - 1)
	{
		zu = get(x, y - 1);
		zd = zc + zc - zu;
	}
	else
	{
		zd = get(x, y + 1);
		zu = get(x, y - 1);
	}

	return vector3df(s * 2 * (zl - zr), 4, s * 2 * (zd - zu)).normalize();
}
*/
