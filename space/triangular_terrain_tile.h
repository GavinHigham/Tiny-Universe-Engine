#ifndef TRIANGULAR_TERRAIN_TILE_H
#define TRIANGULAR_TERRAIN_TILE_H
#include "glla.h"
#include "buffer_group.h"
#include "math/bpos.h"
#include "open-simplex-noise-in-c/open-simplex-noise.h"
#include "mesh.h"
#include <stdbool.h>

//Heightmap function pointers.
typedef float (*height_map_func)(vec3, vec3 *);
typedef vec3 (*position_map_func)(vec3);
typedef struct triangular_terrain_tile tri_tile;

struct tri_tile_vertex {
	vec3 position, normal, color;
	float tx[2];
};

//A vertex for the tile as a whole, not the underlying mesh.
struct tri_tile_big_vertex {
	vec3 position;
	float tx[2];
};

//Triangular terrain tile.
struct triangular_terrain_tile {
	struct tri_tile_vertex *mesh;
	GLuint vao, ibo, mesh_buffer;
	vec3 override_col;
	int num_vertices;
	int num_indices;
	int num_rows;
	//These are the three outermost vertices of the entire triangular tile (pre-deformation).
	struct tri_tile_big_vertex big_vertices[3];
	//Position of the triangular tile's centroid, arithmetic mean of the three big vertices.
	vec3 centroid;
	//Max of distance of each vertex to centroid, for culling.
	float radius;
	//Offset from planet bpos.
	//This is really confusing, I need a better name for it.
	qvec3 offset;
	//Called at the end of init, passing the new tile and the provided context.
	void (*finishing_touches)(tri_tile *, void *);
	void  *finishing_touches_context;
	//Is this tile buffered to the GPU yet?
	bool buffered;
	//Has init_triangular_tile been called on this yet?
	bool is_init;
	//int depth;
	int tile_index;
};

void tri_tile_raycast_test();

tri_tile * tri_tile_new(const struct tri_tile_big_vertex big_vertices[3]);
//tri_tile * tri_tile_new(vec3 vertices[3]);
tri_tile * tri_tile_init(tri_tile *t, bpos_origin sector, int num_rows, void (finishing_touches)(tri_tile *, void *), void *finishing_touches_context);
void tri_tile_deinit(tri_tile *t);
void tri_tile_free(tri_tile *t);

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
int tri_tile_mesh_init(struct tri_tile_vertex mesh[], int num_rows, struct tri_tile_big_vertex big_vertices[3]);

//Returns the depth of a ray cast into the tile t, or infinity if there is no intersection.
float tri_tile_raycast_depth(tri_tile *t, vec3 start, vec3 dir);

//Buffers the position, normal and color buffers of a terrain struct onto the GPU.
void tri_tile_buffer(tri_tile *t);

//Returns the average of two of t's three big verts, indexed by v1 and v2.
struct tri_tile_big_vertex tri_tile_get_big_vert_average(tri_tile *t, int v1, int v2);

//Using height, take position and distort it along the basis vectors, and compute its normal.
//height: A heightmap function which will affect the final position of the vertex along the basis_y vector.
//basis x, basis_y, basis_z: Basis vectors for the vertex.
//position: In/Out, the starting and ending position of the vertex.
//normal: Output for the normal of the vertex.
//Returns the "height", or displacement along basis_y.
float tri_tile_vertex_position_and_normal(height_map_func height, vec3 basis_x, vec3 basis_y, vec3 basis_z, float epsilon, vec3 *position, vec3 *normal);

//TODO(Gavin): Add comment here, maybe rename if I want to make it public.
GLuint get_shared_tri_tile_indices_buffer_object(int num_rows);
//TODO(Gavin): Remove this, I'm only making it public to check the values in proctri_scene
GLuint **get_shared_tri_tile_indices(int num_rows);

#endif