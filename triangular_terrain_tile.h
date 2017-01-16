#ifndef PROCEDURAL_TERRAIN_H
#define PROCEDURAL_TERRAIN_H
#include <glla.h>
#include <stdbool.h>
#include "buffer_group.h"
#include "terrain_types.h"

tri_tile * new_tri_tile();
tri_tile * init_tri_tile(tri_tile *t, vec3 vertices[3], int num_rows, vec3 up, vec3 spos, float srad);
void deinit_tri_tile(tri_tile *t);
void free_tri_tile(tri_tile *t);

//These two are identical to the ones in procedural_terrain, I just needed to rename to avoid duplicate symbols, for now.
float tri_height_map(vec3 pos);
float tri_height_map_flat(vec3 pos);

tri_tile * gen_tri_tile_vertices_and_normals(tri_tile *t, height_map_func height);

int num_tri_tile_indices(int num_rows);
int num_tri_tile_vertices(int num_rows);

//For n rows of triangle strips, the array of indices must be of length n^2 + 3n.
//The index buffer for a tile with n+1 rows contains the index buffer for a tile with n rows.
//The start_row parameter lets you start on a particular row so you can expand an existing buffer.
//Rows are created from smallest to largest.
int tri_tile_indices(GLuint indices[], int num_rows, int start_row);

//For n rows of triangle strips, the array of vertices must be of length (n+2)(n+1)/2
int tri_tile_vertices(vec3 vertices[], int num_rows, vec3 a, vec3 b, vec3 c);

void reproject_vertices_to_spherical(vec3 vertices[], int num_vertices, vec3 spos, float srad);

void subdiv_tri_tile(tri_tile *in, tri_tile *out[DEFAULT_NUM_TRI_TILE_DIVS]);
//Buffers the position, normal and color buffers of a terrain struct onto the GPU.
void buffer_tri_tile(tri_tile *t);

#endif