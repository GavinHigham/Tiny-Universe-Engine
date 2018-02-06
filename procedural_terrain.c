#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <GL/glew.h>
#include "glla.h"
#include "procedural_terrain.h"
#include "open-simplex-noise-in-c/open-simplex-noise.h"
#include "math/utility.h"
#include "buffer_group.h"
#include "macros.h"
#include "space_scene.h"

//If I add margins to all my heightmaps later then I can avoid A LOT of work.
extern int PRIMITIVE_RESTART_INDEX;
extern struct osn_context *osnctx;

//Defines a height map.
//Given an x and z coordinate, returns a vector including a new y coordinate.
//Possibly returns changed x and z as well.
float height_map1(vec3 pos)
{
	float height = 0;
	int octaves = 8;
	for (int i = 1; i <= octaves; i++)
	 	height += i*sin(pos.z/i) + i*sin(pos.x/i);
	return height;
}

float height_map2(vec3 pos)
{
	float height = 0;
	int octaves = 3;
	float amplitude = 50 * pow(2, octaves);
	float sharpness = 2;
	for (int i = 0; i < octaves; i++) {
		amplitude /= 2;
		height += amplitude * pow(open_simplex_noise2(osnctx, pos.x/amplitude, pos.z/amplitude), sharpness);
	}
	return height;
}

float height_map3(vec3 pos)
{
	float height = 0;
	int octaves = 3;
	float amplitude = 50 * pow(2, octaves);
	float sharpness = 2;
	for (int i = 0; i < octaves; i++) {
		amplitude /= 2;
		height += amplitude * pow(open_simplex_noise3(osnctx, pos.x/amplitude, pos.y/amplitude, pos.y/amplitude), sharpness);
	}
	return height;
}

float height_map_flat(vec3 pos)
{
	return 1;
}

//Cheap trick to get normals, should replace with something faster eventually.
vec3 height_map_normal(height_map_func height, vec3 pos)
{
	float epsilon = 0.001;
	vec3 pos1 = {pos.x + epsilon, pos.y, pos.z};
	vec3 pos2 = {pos.x, pos.y, pos.z + epsilon};
	pos.y  = height(pos);
	pos1.y = height(pos1);
	pos2.y = height(pos2);

	return vec3_normalize(vec3_cross(pos2 - pos, pos1 - pos));
}

//Frees the dynamic storage and OpenGL objects held by a terrain struct.
//After calling, *t should be considered invalid, and not used again.
void free_terrain(struct terrain *t)
{
	free(t->positions);
	free(t->normals);
	free(t->colors);
	free(t->indices);
	glDeleteVertexArrays(1, &t->bg.vao);
	glDeleteBuffers(1, &t->bg.ibo);
	glDeleteBuffers(LENGTH(t->bg.buffer_handles), t->bg.buffer_handles);
}

/*
bool terrain_in_frustrum(struct terrain *t, amat4 camera, float projection_matrix[16])
{
	//TODO
	return true;
}
*/

//Generates an initial heightmap terrain and associated normals.
void populate_terrain(struct terrain *t, vec3 world_pos, height_map_func height)
{
	t->pos = world_pos;
	//Generate vertices.
	for (int i = 0; i < t->numrows; i++) {
		for (int j = 0; j < t->numcols; j++) {
			int offset = (t->numcols * i) + j;
			vec3 pos = world_pos;
			pos.y = height(world_pos);
			float intensity = pow(pos.y/100.0, 2);
			vec3 color = {intensity, intensity, intensity};
			vec3 norm = height_map_normal(height, pos);
			t->positions[offset] = pos;
			t->normals[offset] = norm;
			t->colors[offset] = color;
		}
	}
}

//For n rows of triangle strips, the array of indices must be of length n^2 + 3n.
int triangle_tile_indices(GLuint indices[], int numrows)
{
	int written = 0;
	for (int i = 0; i < numrows; i++) {
		int start_index = ((i+2)*(i+1))/2;
		for (int j = start_index; j < start_index + i + 1; j++) {
			indices[written++] = j;
			indices[written++] = j - i - 1;
		}
		indices[written++] = start_index + i + 1;
		indices[written++] = PRIMITIVE_RESTART_INDEX;
	}
	return written;
}

//For n rows of triangle strips, the array of vertices must be of length (n+2)(n+1)/2
int triangle_tile_vertices(vec3 vertices[], int numrows, vec3 a, vec3 b, vec3 c)
{
	int written = 0;
	vertices[written++] = a;
	for (int i = 1; i <= numrows; i++) {
		float f1 = (float)i/(numrows);
		vec3 left = vec3_lerp(a, b, f1);
		vec3 right = vec3_lerp(a, c, f1);
		for (int j = 0; j <= i; j++) {
			float f2 = (float)j/(i);
			vertices[written++] = vec3_lerp(left, right, f2); 
		}
	}
	return written;
}

int square_tile_indices(GLuint indices[], int numrows, int numcols)
{
	int written = 0;
	for (int i = 0; i < (numrows - 1); i++) {
		int j = 0;
		int col_offset = i*(numcols*2+1);
		for (j = 0; j < numcols; j++) {
			indices[col_offset + j*2]     = i*numcols + j + numcols;
			indices[col_offset + j*2 + 1] = i*numcols + j;
		}
		indices[col_offset + j*2] = PRIMITIVE_RESTART_INDEX;
	}
	return written; //TODO, make this return the actual number written.
}

//Creates a terrain struct, including handles for various OpenGL objects
//and storage for the positions, normals, colors, and indices.
//Should be freed by the caller, using free_terrain.
//Later note: Why is the number of rows and columns referring to the number of vertices?
struct terrain new_terrain(int numrows, int numcols)
{
	numrows++;
	numcols++;
	struct terrain tmp;
	tmp.in_frustrum = true;
	tmp.bg.index_count = (2 * numcols + 1) * (numrows - 1);
	tmp.atrlen = sizeof(vec3) * numrows * numcols;
	tmp.indlen = sizeof(GLuint) * tmp.bg.index_count;
	tmp.positions = (vec3 *)malloc(tmp.atrlen);
	tmp.normals   = (vec3 *)malloc(tmp.atrlen);
	tmp.colors    = (vec3 *)malloc(tmp.atrlen);
	tmp.indices = (GLuint *)malloc(tmp.indlen);
	tmp.numrows = numrows;
	tmp.numcols = numcols;
	if (!tmp.positions || !tmp.normals || !tmp.colors || !tmp.indices)
		printf("Malloc didn't work lol\n");
	tmp.bg.primitive_type = GL_TRIANGLE_STRIP;
	glGenVertexArrays(1, &tmp.bg.vao);
	glGenBuffers(LENGTH(tmp.bg.buffer_handles), tmp.bg.buffer_handles);
	glGenBuffers(1, &tmp.bg.ibo);
	glBindVertexArray(tmp.bg.vao);
	setup_attrib_for_draw(effects.forward.vPos,    tmp.bg.vbo, GL_FLOAT, 3);
	setup_attrib_for_draw(effects.forward.vNormal, tmp.bg.nbo, GL_FLOAT, 3);
	setup_attrib_for_draw(effects.forward.vColor,  tmp.bg.cbo, GL_FLOAT, 3);
	//Generate indices
	square_tile_indices(tmp.indices, numrows, numcols);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmp.bg.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tmp.indlen, tmp.indices, GL_STATIC_DRAW);
	return tmp;
}

//Creates a terrain struct, including handles for various OpenGL objects
//and storage for the positions, normals, colors, and indices.
//Should be freed by the caller, using free_terrain.
struct terrain new_triangular_terrain(int numrows)
{
	struct terrain tmp;
	tmp.in_frustrum = true;
	tmp.buffered = false;
	tmp.bg.index_count = numrows*numrows + 3*numrows;
	tmp.atrlen = sizeof(vec3) * ((numrows + 2) * (numrows + 1)) / 2;
	tmp.indlen = sizeof(GLuint) * tmp.bg.index_count;
	tmp.positions = (vec3 *)malloc(tmp.atrlen);
	tmp.normals   = (vec3 *)malloc(tmp.atrlen);
	tmp.colors    = (vec3 *)malloc(tmp.atrlen);
	tmp.indices = (GLuint *)malloc(tmp.indlen);
	tmp.numrows = numrows;
	tmp.numcols = numrows;
	if (!tmp.positions || !tmp.normals || !tmp.colors || !tmp.indices)
		printf("Malloc didn't work lol\n");
	tmp.bg.primitive_type = GL_TRIANGLE_STRIP;
	glGenVertexArrays(1, &tmp.bg.vao);
	glGenBuffers(LENGTH(tmp.bg.buffer_handles), tmp.bg.buffer_handles);
	glGenBuffers(1, &tmp.bg.ibo);
	glBindVertexArray(tmp.bg.vao);
	setup_attrib_for_draw(effects.forward.vPos,    tmp.bg.vbo, GL_FLOAT, 3);
	setup_attrib_for_draw(effects.forward.vNormal, tmp.bg.nbo, GL_FLOAT, 3);
	setup_attrib_for_draw(effects.forward.vColor,  tmp.bg.cbo, GL_FLOAT, 3);
	//Generate indices

	int numindices = triangle_tile_indices(tmp.indices, numrows);
	assert(tmp.bg.index_count == numindices);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmp.bg.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tmp.indlen, tmp.indices, GL_STATIC_DRAW);
	return tmp;
}

//Generates an initial heightmap terrain and associated normals.
void populate_triangular_terrain(struct terrain *t, vec3 points[3], height_map_func height)
{
	t->pos = (points[0] + points[1] + points[2]) * 1.0/3.0;
	for (int i = 0; i < 3; i++)
		t->points[i] = points[i];
	int numverts = triangle_tile_vertices(t->positions, t->numrows, points[0], points[1], points[2]);
	assert(t->atrlen/sizeof(vec3) == numverts);
	//Generate vertices.
	for (int i = 0; i < numverts; i++) {
		vec3 *ppos = &t->positions[i];
		ppos->y = height(*ppos);
		float intensity = pow(ppos->y/100.0, 2);
		t->normals[i] = height_map_normal(height, *ppos);
		t->colors[i] = (vec3){intensity, intensity, intensity};
	}
}

void subdiv_triangle_terrain(struct terrain *in, struct terrain *out[NUM_TRI_DIVS])
{
	vec3 new_points[] = {
		vec3_lerp(in->points[0], in->points[1], 0.5),
		vec3_lerp(in->points[0], in->points[2], 0.5),
		vec3_lerp(in->points[1], in->points[2], 0.5)
	};

	//populate_triangular_terrain(out[0], (vec3[3]){in->points[0], in->points[1], in->points[2]}, height_map2);
	populate_triangular_terrain(out[0], (vec3[3]){in->points[0], new_points[0], new_points[1]}, height_map2);
	populate_triangular_terrain(out[1], (vec3[3]){new_points[0], in->points[1], new_points[2]}, height_map2);
	populate_triangular_terrain(out[2], (vec3[3]){new_points[0], new_points[2], new_points[1]}, height_map2);
	populate_triangular_terrain(out[3], (vec3[3]){new_points[1], new_points[2], in->points[2]}, height_map2);

	printf("Created 4 new terrains from terrain %p\n", in);
}

// //Use only terrain grids with odd numbers of tiles!
// void terrain_grid_make_current(struct terrain_grid *tg, int numx, int numz, vec3 pos)
// {
// 	tgpos(tg, 0, 0);
// 	for (int i = 0; i < tg->numx; i++) {
// 		for (int j = 0; j < tg->numz; j++) {
// 			struct terrain *expected = tgpos(NULL, i - tg->numx/2.0, j - tg->numz/2.0);
// 			if (memcmp(&(tg->ts[i + tg->numx*j].pos), &(expected->pos), sizeof(vec3)) != 0) {
// 				populate_terrain(&(tg->ts[i + tg->numx*j]), )
// 			}
// 		}
// 	}
// }

//Buffers the position, normal and color buffers of a terrain struct onto the GPU.
void buffer_terrain(struct terrain *t)
{
	glBindVertexArray(t->bg.vao);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.vbo);
	glBufferData(GL_ARRAY_BUFFER, t->atrlen, t->positions, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.nbo);
	glBufferData(GL_ARRAY_BUFFER, t->atrlen, t->normals, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.cbo);
	glBufferData(GL_ARRAY_BUFFER, t->atrlen, t->colors, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.ibo);
	t->buffered = true;
}