#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <GL/glew.h>
#include <glalgebra.h>
#include "procedural_terrain.h"
#include "open-simplex-noise-in-c/open-simplex-noise.h"
#include "math/utility.h"
#include "buffer_group.h"
#include "macros.h"
#include "render.h"

//If I add margins to all my heightmaps later then I can avoid A LOT of work.

const float ACCELERATION_DUE_TO_GRAVITY = 9.81;
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
	vec3 pos1 = {{pos.x + epsilon, pos.y, pos.z}};
	vec3 pos2 = {{pos.x, pos.y, pos.z + epsilon}};
	pos.y  = height(pos);
	pos1.y = height(pos1);
	pos2.y = height(pos2);

	return vec3_normalize(vec3_cross(vec3_sub(pos2, pos), vec3_sub(pos1, pos)));
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

//Calculates an array index for t->positions.
//Will return a pointer to the vec3 nearest <fx, fz>, wrapping around as on a torus.
//To initialize, call with t as the struct you wish to navigate, and any fx/fz.
//On subsequent calls, leave t as NULL and pass actual fx and fz coordinates.
vec3 * tpos(struct terrain *t, float fx, float fz)
{
	unsigned int x = fmod(fmod(fx, t->numcols) + t->numcols, t->numcols);
	unsigned int z = fmod(fmod(fz, t->numrows) + t->numrows, t->numrows);
	unsigned int coord = x + z*t->numcols;
	return &(t->positions[coord]);
}

//Calculates an array index for t->normals.
//Will return a pointer to the vec3 nearest <fx, fz>, wrapping around as on a torus.
//To initialize, call with t as the struct you wish to navigate, and any fx/fz.
//On subsequent calls, leave t as NULL and pass actual fx and fz coordinates.
vec3 * tnorm(struct terrain *t, float fx, float fz)
{
	unsigned int x = fmod(fmod(fx, t->numcols) + t->numcols, t->numcols);
	unsigned int z = fmod(fmod(fz, t->numrows) + t->numrows, t->numrows);
	unsigned int coord = x + z*t->numcols;
	return &(t->normals[coord]);
}

//State needed for a raindrop to be simulated on some heightmap terrain.
struct raindrop {
	vec3 pos;
	vec3 vel;
	float load; //Amount of sediment drop is carrying, should be < capacity.
	int steps; //Number of steps this raindrop has been simulated for.
};

struct raindrop_config {
	float mass;
	float friction; //Kinetic friction coefficient between drop and terrain.
	float capacity; //Maximum amount of sediment this raindrop can carry.
	float speedfloor; //Drop is considered immobile at or below this speed.
	int max_steps; //Maximum number of steps this raindrop will be simulated for.
};

int simulate_raindrop(struct terrain *t, struct raindrop_config rc, float x, float z)
{
	tpos(t, 0, 0);
	//Raindrop starts at x, y.
	struct raindrop r = {.pos = {{x, 0, z}}, .vel = {{0, 0, 0}}, .load = 0, .steps = 0};
	float deltah = 1;
	float timescale = 1;
	int hit_steplimit = 0;
	do {
		vec3 *p  = tpos(t, r.pos.x,   r.pos.z);
		vec3 *c1 = tpos(t, r.pos.x+1, r.pos.z);
		vec3 *c2 = tpos(t, r.pos.x+1, r.pos.z+1);
		vec3 *c3 = tpos(t, r.pos.x,   r.pos.z+1);
		vec3 grad = (vec3){{
			p->y + c3->y - c1->y - c2->y,
			0,
			c2->y + c3->y - p->y - c1->y
		}};
		if (vec3_mag(grad) > 0)
			grad = vec3_normalize(grad);

		//Force of gravity pulls down, some becomes normal force, some becomes a force along the slope.
		//If the downward velocity was higher than would keep it on the surface of the next slope segment,
		//calculate the deceleration needed to keep it on the slope, and add it to the normal force.
		//Add up all the forces that acted on the drop in the last segment, apply them, and determine the new
		//direction of travel by normalizing the velocity vector.
		float Fg = ACCELERATION_DUE_TO_GRAVITY * rc.mass;
		float b = sqrt(1 + deltah*deltah);
		float Fn = Fg / b;
		float Af = -Fn * (rc.friction / rc.mass); //Acceleration (deceleration) due to friction.
		float Ag = deltah / b; //Acceleration due to gravity aligned down the gradient.
		float A = Ag + Af; //Total acceleration experienced by the drop.
		vec3 a_vec = {{
			grad.x*A,
			grad.y*A,
			grad.z*A
		}};
		r.vel = vec3_add(r.vel, vec3_scale(a_vec, timescale));
		float vmag = vec3_mag(r.vel);
		if (vmag == 0)
			timescale = 1;
		else
			timescale = 1/vmag;
		assert(timescale != NAN);
		r.pos = vec3_add(r.pos, vec3_scale(r.vel, timescale)); //Move the raindrop by one unit in its velocity direction.
		// r.pos  = vec3_add(r.pos, grad); //Move the raindrop along the gradient.
		vec3 *newp = tpos(t, r.pos.x, r.pos.z);
		deltah = newp->y - p->y;
		if (deltah < 0) { //The drop moves downhill
			float sediment = fmin(rc.capacity - r.load, -deltah);
			p->y -= sediment;
			r.load += sediment;
		} else {
			float sediment = fmin(r.load, deltah);
			p->y += sediment;
			r.load -= sediment;
		}
		r.steps++;
		if (r.steps >= rc.max_steps) {
			hit_steplimit = 1;
			break;
		}
	} while (vec3_mag(r.vel) > rc.speedfloor);
	vec3 *finalp = tpos(t, r.pos.x, r.pos.y);
	finalp->y += r.load; //Simulate evaporation.
	// if (finalp == t->positions)
	// 	printf("Finished at [0, 0] :/\n");
	//Picks up sediment controlled by ground cohesion, up to its capacity
	//Ground that is eroded becomes more cohesive
	//Raindrop rolls downhill
	//Raindrop picks up speed as it descends, slowed by friction
	//When raindrop becomes slower than a certain threshold, it evaporates, leaving sediment
	//Velocity is determined by accelerating raindrop according to gradient of surface
	//Ground that has received sediment becomes less cohesive
	return hit_steplimit;
}

//Updates the t->normals
void recalculate_terrain_normals_expensive(struct terrain *t)
{
	for (int x = 0; x < t->numcols; x++) {
		for (int z = 0; z < t->numrows; z++) {
			vec3 p0 = *tpos(t, x,   z);
			vec3 *p1 = tpos(t, x+1, z);
			vec3 *p5 = tpos(t, x-1, z);
			vec3 *p3 = tpos(t, x,   z+1);
			vec3 *p7 = tpos(t, x,   z-1);
			vec3 *p2 = tpos(t, x+1, z+1);
			vec3 *p4 = tpos(t, x-1, z+1);
			vec3 *p6 = tpos(t, x-1, z-1);
			vec3 *p8 = tpos(t, x+1, z-1);

			*tnorm(t, x, z) = vec3_normalize(
				vec3_add(
					vec3_add(
						vec3_cross(vec3_sub(*p1, p0), vec3_sub(*p3, p0)),
						vec3_cross(vec3_sub(*p5, p0), vec3_sub(*p7, p0))
					),
					vec3_add(
						vec3_cross(vec3_sub(*p2, p0), vec3_sub(*p4, p0)),
						vec3_cross(vec3_sub(*p6, p0), vec3_sub(*p8, p0))
					)
				)
			);
		}
	}
}

//Updates the t->normals
void recalculate_terrain_normals_cheap(struct terrain *t)
{
	for (int x = 0; x < t->numcols; x++) {
		for (int z = 0; z < t->numrows; z++) {
			vec3 *p1 = tpos(t, x+1, z);
			vec3 *p5 = tpos(t, x-1, z);
			vec3 *p3 = tpos(t, x, z+1);
			vec3 *p7 = tpos(t, x, z-1);

			*tnorm(t, x, z) = vec3_normalize(vec3_cross(vec3_sub(*p1, *p5), vec3_sub(*p3, *p7)));
		}
	}
}

void erode_terrain(struct terrain *t, int iterations)
{
	struct raindrop_config rc = {
		.mass = 1.0,
		.friction = 0.8, //Kinetic friction coefficient between drop and terrain.
		.capacity = .7, //Maximum amount of sediment this raindrop can carry.
		.speedfloor = 0.07, //Drop is considered immobile at or below this speed.
		.max_steps = 1000 //Maximum number of steps this raindrop will be simulated for.
	};

	int steplimit_drops = 0;
	for (int i = 0; i < iterations; i++)
		steplimit_drops += simulate_raindrop(t, rc, rand_float()*(t->numcols-1), rand_float()*(t->numrows-1));
	printf("%i drops hit the steplimit this iteration.\n", steplimit_drops);
}

struct terrain_grid {
	struct terrain *ts;
	int numx;
	int numz;
};

//Calculates an array index for a 2d grid of terrains.
//Will return a pointer to the terrain struct nearest <fx, fy>, wrapping around as on a torus.
//To initialize, call with ts as the struct array you wish to navigate, and any fx/fy.
//On subsequent calls, leave ts as NULL and pass actual fx and fy coordinates.
struct terrain * tgpos(struct terrain_grid *tg, float fx, float fy)
{
	static struct terrain_grid *tref = NULL;
	static float tilex;
	static float tilez;
	if (tg == NULL) {
		unsigned int x = fmod(fmod(fx/tilex, tref->numx) + tref->numx, tref->numx);
		unsigned int y = fmod(fmod(fy/tilez, tref->numz) + tref->numz, tref->numz);
		unsigned int coord = x + y*tref->numx;
		return &(tref->ts[coord]);
	}
	else {
		tref = tg;
		tilex = tg->ts->numcols;
		tilez = tg->ts->numrows;
		return NULL;
	}
}

vec3 nearest_terrain_origin(vec3 pos, float terrain_width, float terrain_depth)
{
	unsigned int x = fmod(fmod(pos.x, terrain_width) + terrain_width, terrain_width);
	unsigned int z = fmod(fmod(pos.z, terrain_depth) + terrain_depth, terrain_depth);
	return (vec3){{pos.x - x, 0, pos.z - z}};
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
			vec3 color = {{intensity, intensity, intensity}};
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
void populate_triangular_terrain(struct terrain *t, vec3 world_pos, float base, height_map_func height)
{
	t->pos = world_pos;
	vec3 a = vec3_add(world_pos, (vec3){{0.0,       0.0,  base*(sqrt(3.0)/4.0)}});
	vec3 b = vec3_add(world_pos, (vec3){{-base/2.0, 0.0, -base*(sqrt(3.0)/4.0)}});
	vec3 c = vec3_add(world_pos, (vec3){{ base/2.0, 0.0, -base*(sqrt(3.0)/4.0)}});
	int numverts = triangle_tile_vertices(t->positions, t->numrows, a, b, c);
	assert(t->atrlen/sizeof(vec3) == numverts);
	//Generate vertices.
	for (int i = 0; i < numverts; i++) {
		vec3 *ppos = &t->positions[i];
		ppos->y = height(*ppos);
		float intensity = pow(ppos->y/100.0, 2);
		t->normals[i] = height_map_normal(height, *ppos);
		t->colors[i] = (vec3){{intensity, intensity, intensity}};
	}
}

//Generates an initial heightmap terrain and associated normals.
void populate_triangular_terrain2(struct terrain *t, vec3 world_pos, float base, height_map_func height)
{
	t->pos = world_pos;
	vec3 a = vec3_add(world_pos, (vec3){{0.0,       0.0,  base*(sqrt(3.0)/4.0)}});
	vec3 b = vec3_add(world_pos, (vec3){{-base/2.0, 0.0, -base*(sqrt(3.0)/4.0)}});
	vec3 c = vec3_add(world_pos, (vec3){{ base/2.0, 0.0, -base*(sqrt(3.0)/4.0)}});
	int numverts = triangle_tile_vertices(t->positions, t->numrows, a, b, c);
	assert(t->atrlen/sizeof(vec3) == numverts);
	//Generate vertices.
	for (int i = 0; i < numverts; i++) {
		vec3 *ppos = &t->positions[i];
		ppos->y = height(*ppos);
		t->normals[i] = height_map_normal(height, *ppos);
		float intensity = pow(ppos->y/100.0, 2);
		t->colors[i] = (vec3){{intensity, intensity, intensity}};
	}
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
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.vbo);
	glBufferData(GL_ARRAY_BUFFER, t->atrlen, t->positions, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.nbo);
	glBufferData(GL_ARRAY_BUFFER, t->atrlen, t->normals, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.cbo);
	glBufferData(GL_ARRAY_BUFFER, t->atrlen, t->colors, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.ibo);
}