#ifndef TRIANGULAR_TERRAIN_TILE_H
#define TRIANGULAR_TERRAIN_TILE_H
#include <stdbool.h>
#include "glla.h"
#include "buffer_group.h"
#include "dynamic_terrain_types.h"

tri_tile * new_tri_tile();
//tri_tile * init_tri_tile(tri_tile *t, vec3 vertices[3], int num_rows, vec3 up, vec3 spos, float srad);
tri_tile * init_tri_tile(tri_tile *t, vec3 vertices[3], space_sector sector, int num_rows, void (finishing_touches)(tri_tile *, void *), void *finishing_touches_context);
void deinit_tri_tile(tri_tile *t);
void free_tri_tile(tri_tile *t);

//These two are identical to the ones in procedural_terrain, I just needed to rename to avoid duplicate symbols, for now.
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