#ifndef DYNAMIC_TERRAIN_TYPES_H
#define DYNAMIC_TERRAIN_TYPES_H
#include <stdbool.h>
#include <GL/glew.h>
#include "glla.h"
#include "buffer_group.h"
#include "terrain_constants.h"
#include "math/space_sector.h"

//Heightmap function pointers.
typedef float (*height_map_func)(vec3);
typedef vec3 (*position_map_func)(vec3);
typedef struct triangular_terrain_tile tri_tile;

//Triangular terrain tile.
struct triangular_terrain_tile {
	//Cached computed positions, colors, and normals.
	vec3 *positions;
	vec3 *colors;
	vec3 *normals;
	GLuint *indices;
	int num_vertices;
	int num_indices;
	int num_rows;
	//Not to be confused with render geometry,
	//these are the three outermost vertices of the entire triangular tile (pre-deformation).
	vec3 tile_vertices[3];
	//Position of the triangular tile's centroid, arithmetic mean of tile_vertices.
	vec3 pos;
	space_sector sector;
	//Called at the end of init, passing the new tile and the provided context.
	void (*finishing_touches)(tri_tile *, void *);
	void  *finishing_touches_context;
	//Information needed for rendering.
	struct buffer_group bg;
	//Is this tile buffered to the GPU yet?
	bool buffered;
	//Has init_triangular_tile been called on this yet?
	bool is_init;
};

#endif