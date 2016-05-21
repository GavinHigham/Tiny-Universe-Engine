#ifndef PROCEDURAL_TERRAIN_H
#define PROCEDURAL_TERRAIN_H
#include "math/vector3.h"
#include "buffer_group.h"

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
};
//struct buffer_group buffer_grid(int numrows, int numcols);
vec3 height_map1(float x, float z);
vec3 height_map_normal1(float x, float z);
struct terrain new_terrain(int numrows, int numcols);
void free_terrain(struct terrain *t);
void buffer_terrain(struct terrain *t);
void populate_terrain(struct terrain *t, vec3 (*height_map)(float x, float y), vec3 (*height_map_normal)(float x, float y));
void erode_terrain(struct terrain *t, int iterations);
void recalculate_terrain_normals(struct terrain *t);

#endif