#ifndef TRIANGULAR_TERRAIN_TILE_H
#define TRIANGULAR_TERRAIN_TILE_H
#include <stdbool.h>
#include "glla.h"
#include "buffer_group.h"
#include "math/space_sector.h"

//Heightmap function pointers.
typedef float (*height_map_func)(vec3, vec3 *);
typedef vec3 (*position_map_func)(vec3);
typedef struct triangular_terrain_tile tri_tile;

//Triangular terrain tile.
struct triangular_terrain_tile {
	//Cached computed positions, colors, and normals.
	vec3 *positions;
	vec3 *colors;
	vec3 *normals;
	vec3 override_col;
	GLuint *indices;
	int num_vertices;
	int num_indices;
	//Not to be confused with render geometry,
	//these are the three outermost vertices of the entire triangular tile (pre-deformation).
	vec3 tile_vertices[3];
	//Position of the triangular tile's centroid, arithmetic mean of tile_vertices.
	vec3 centroid;
	vec3 normal;
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
	int depth;
};

tri_tile * new_tri_tile();
tri_tile * init_tri_tile(tri_tile *t, vec3 vertices[3], space_sector sector, int num_rows, void (finishing_touches)(tri_tile *, void *), void *finishing_touches_context);
void deinit_tri_tile(tri_tile *t);
void free_tri_tile(tri_tile *t);

float tri_height_map(vec3 pos);
float tri_height_map_flat(vec3 pos);

int num_tri_tile_indices(int num_rows);
int num_tri_tile_vertices(int num_rows);

//For n rows of triangle strips, the array of indices must be of length n^2 + 3n.
//The index buffer for a tile with n+1 rows contains the index buffer for a tile with n rows.
//The start_row parameter lets you start on a particular row so you can expand an existing buffer.
//Rows are created from smallest to largest.
int tri_tile_indices(GLuint indices[], int num_rows, int start_row);

//For n rows of triangle strips, the array of vertices must be of length (n+2)(n+1)/2
int tri_tile_vertices(vec3 vertices[], int num_rows, vec3 a, vec3 b, vec3 c);

//Buffers the position, normal and color buffers of a terrain struct onto the GPU.
void buffer_tri_tile(tri_tile *t);

//Using height, take position and distort it along the basis vectors, and compute its normal.
//height: A heightmap function which will affect the final position of the vertex along the basis_y vector.
//basis x, basis_y, basis_z: Basis vectors for the vertex.
//position: In/Out, the starting and ending position of the vertex.
//normal: Output for the normal of the vertex.
//Returns the "height", or displacement along basis_y.
float tri_tile_vertex_position_and_normal(height_map_func height, vec3 basis_x, vec3 basis_y, vec3 basis_z, float epsilon, vec3 *position, vec3 *normal);

#endif