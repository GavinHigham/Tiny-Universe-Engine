#ifndef TERRAIN_CONSTANTS_H
#define TERRAIN_CONSTANTS_H

enum {
	//A triangular tile is divided in "triforce" fashion, that is,
	//by dividing along the edges of a triangle whose vertices are the bisection of each original tile edge.
	DEFAULT_NUM_TRI_TILE_DIVS = 4, //Number of tiles a triangle is split into.
	TRI_BASE_LEN = 10000,
	NUM_ICOSPHERE_FACES = 20,
	TERRAIN_AMPLITUDE = 40
};

#endif
