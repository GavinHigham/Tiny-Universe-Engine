#ifndef TERRAIN_CONSTANTS_H
#define TERRAIN_CONSTANTS_H

enum {
	//The more rows, the fewer draw calls, and the fewer primitive restart indices hurting memory locality.
	//The fewer rows, the fewer overall vertices, and the better overall culling efficiency.
	//If I keep it as a power-of-two, I can avoid using spherical linear interpolation, and it will be faster.
	DEFAULT_NUM_TRI_TILE_ROWS = 128,
	//A triangular tile is divided in "triforce" fashion, that is,
	//by dividing along the edges of a triangle whose vertices are the bisection of each original tile edge.
	DEFAULT_NUM_TRI_TILE_DIVS = 4, //Number of tiles a triangle is split into.
	TRI_BASE_LEN = 10000,
	PIXELS_PER_TRI = 30,
	MAX_SUBDIVISIONS = 5, //Max subdivision depth.
	YIELD_AFTER_DIVS = 50, //Max number of split operations per tree traversal.
	NUM_ICOSPHERE_FACES = 20,
	TERRAIN_AMPLITUDE = 50
};

#endif
