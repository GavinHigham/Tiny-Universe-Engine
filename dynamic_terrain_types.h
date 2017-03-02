#ifndef DYNAMIC_TERRAIN_TYPES_H
#define DYNAMIC_TERRAIN_TYPES_H
#include <stdbool.h>
#include <GL/glew.h>
#include "glla.h"
#include "buffer_group.h"
#include "terrain_constants.h"

//Heightmap function pointers.
typedef float (*height_map_func)(vec3);
typedef vec3 (*position_map_func)(vec3);

//Triangular terrain tile.
typedef struct {
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
	height_map_func height;
	//Used for normals calculation.
	vec3 up;
	//Bases for x, y, z movement relative to the surface of the triangular tile, y is normal to the tile as a whole.
	vec3 basis_x;
	vec3 basis_y;
	vec3 basis_z;
	//Position of the triangular tile's centroid, arithmetic mean of tile_vertices.
	vec3 pos;
	//Sphere origin and radius, used to appropriately curve tiles.
	vec3 s_origin;
	float s_radius;
	float noise_radius;
	//Information needed for rendering.
	struct buffer_group bg;
	//Is this tile buffered to the GPU yet?
	bool buffered;
	//Has init_triangular_tile been called on this yet?
	bool is_init;
} tri_tile;

#endif