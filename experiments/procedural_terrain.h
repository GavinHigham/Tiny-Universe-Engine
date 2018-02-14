#ifndef PROCEDURAL_TERRAIN_H
#define PROCEDURAL_TERRAIN_H
#include <stdbool.h>
#include "glla.h"
#include "../buffer_group.h"

struct terrain {
	struct buffer_group bg;
	vec3 *positions;
	vec3 *colors;
	vec3 *normals;
	GLuint *indices;
	int atrlen;
	int indlen;
	int numrows;
	int numcols;
	vec3 pos;
	vec3 points[3]; //The three triangular points that make it up if it's a triangular tile.
	bool buffered; //Buffered to the GPU.
	bool in_frustrum;
};

enum {
	//The more rows, the fewer draw calls, and the fewer primitive restart indices hurting memory locality.
	//The fewer rows, the fewer overall vertices, and the better overall culling efficiency.
	NUM_TRI_ROWS = 100,
	NUM_TRI_DIVS = 4
};

typedef float (*height_map_func)(vec3);
//struct buffer_group buffer_grid(int numrows, int numcols);
float height_map1(vec3 pos);
float height_map2(vec3 pos);
float height_map_flat(vec3 pos);
struct terrain new_terrain(int numrows, int numcols);
struct terrain new_triangular_terrain(int numrows);
void free_terrain(struct terrain *t);
void buffer_terrain(struct terrain *t);
void populate_terrain(struct terrain *t, vec3 world_pos, height_map_func);
void populate_triangular_terrain(struct terrain *t, vec3 points[3], height_map_func);
void subdiv_triangle_terrain(struct terrain *in, struct terrain *out[NUM_TRI_DIVS]);
void erode_terrain(struct terrain *t, int iterations);
void recalculate_terrain_normals_cheap(struct terrain *t);
void recalculate_terrain_normals_expensive(struct terrain *t);

#endif