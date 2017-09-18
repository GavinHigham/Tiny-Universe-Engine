#ifndef TERRAIN_CONSTANTS_H
#define TERRAIN_CONSTANTS_H

enum {
	//A triangular tile is divided in "triforce" fashion, that is,
	//by dividing along the edges of a triangle whose vertices are the bisection of each original tile edge.
	DEFAULT_NUM_TRI_TILE_DIVS = 4, //Number of tiles a triangle is split into.
	TRI_BASE_LEN = 10000,
	PIXELS_PER_TRI = 5,
	MAX_SUBDIVISIONS = 5, //Max subdivision depth.
	YIELD_AFTER_DIVS = 50, //Max number of split operations per tree traversal.
	NUM_ICOSPHERE_FACES = 20,
	TERRAIN_AMPLITUDE = 40
};

#endif
