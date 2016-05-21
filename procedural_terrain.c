#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <GL/glew.h>
#include "procedural_terrain.h"
#include "math/vector3.h"
#include "buffer_group.h"
#include "macros.h"
#include "render.h"

VEC3 *gpositions;

extern int PRIMITIVE_RESTART_INDEX;

static float rand_float()
{
	return (float)((double)rand()/RAND_MAX); //Discard precision after the division.
}

VEC3 height_map1(float x, float z)
{
	float height = 0;
	int octaves = 8;
	for (int i = 1; i <= octaves; i++)
	 	height += i*sin(z/i) + i*sin(x/i);
	//height = sqrt((x-50)*(x-50) + (z-50)*(z-50)) - 10;
	return (VEC3){{x, height, z}};
}

//Cheap trick to get normals, should replace with something faster eventually.
VEC3 height_map_normal1(float x, float z)
{
	float delta = 0.0001;
	VEC3 v0 = height_map1(x, z);
	VEC3 v1 = height_map1(x+delta, z);
	VEC3 v2 = height_map1(x, z+delta);

	return vec3_normalize(vec3_cross(vec3_sub(v2, v0), vec3_sub(v1, v0)));
	//return (VEC3){{0, 1, 0}};
}

struct terrain new_terrain(int numrows, int numcols)
{
	struct terrain tmp;
	tmp.bg.index_count = (2 * numcols + 1) * (numrows - 1);
	tmp.atrlen = sizeof(VEC3) * numrows * numcols;
	tmp.indlen = sizeof(GLuint) * tmp.bg.index_count;
	tmp.positions = (VEC3 *)malloc(tmp.atrlen);
	tmp.normals   = (VEC3 *)malloc(tmp.atrlen);
	tmp.colors    = (VEC3 *)malloc(tmp.atrlen);
	tmp.indices = (GLuint *)malloc(tmp.indlen);
	gpositions = tmp.positions;
	tmp.numrows = numrows;
	tmp.numcols = numcols;
	if (!tmp.positions || !tmp.normals || !tmp.colors || !tmp.indices)
		printf("Malloc didn't work lol\n");
	tmp.bg.primitive_type = GL_TRIANGLE_STRIP;
	glGenVertexArrays(1, &tmp.bg.vao);
	glGenBuffers(LENGTH(tmp.bg.buffer_handles), tmp.bg.buffer_handles);
	glGenBuffers(1, &tmp.bg.ibo);
	glBindVertexArray(tmp.bg.vao);
	setup_attrib_for_draw(forward_program.vPos,    tmp.bg.vbo, GL_FLOAT, 3);
	setup_attrib_for_draw(forward_program.vNormal, tmp.bg.nbo, GL_FLOAT, 3);
	setup_attrib_for_draw(forward_program.vColor,  tmp.bg.cbo, GL_FLOAT, 3);
	//Generate indices
	for (int i = 0; i < (numrows - 1); i++) {
		int j = 0;
		int col_offset = i*(numcols*2+1);
		for (j = 0; j < numcols; j++) {
			tmp.indices[col_offset + j*2]     = i*numcols + j + numcols;
			tmp.indices[col_offset + j*2 + 1] = i*numcols + j;
		}
		tmp.indices[col_offset + j*2] = PRIMITIVE_RESTART_INDEX;
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tmp.bg.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tmp.indlen, tmp.indices, GL_STATIC_DRAW);
	return tmp;
}

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

VEC3 * tpos(struct terrain *t, float fx, float fy)
{
	static struct terrain *tref = NULL;
	if (t == NULL) {
		unsigned int x = fmod(fmod(fx, tref->numcols) + tref->numcols, tref->numcols);
		unsigned int y = fmod(fmod(fy, tref->numrows) + tref->numrows, tref->numrows);
		unsigned int coord = x + y*tref->numcols;
		assert(coord >= 0);
		assert(coord < tref->numrows*tref->numcols);
		return &(tref->positions[coord]);
	}
	else {
		tref = t;
		return NULL;
	}
}

VEC3 * tnorm(struct terrain *t, float fx, float fy)
{
	static struct terrain *tref = NULL;
	if (t == NULL) {
		unsigned int x = fmod(fmod(fx, tref->numcols) + tref->numcols, tref->numcols);
		unsigned int y = fmod(fmod(fy, tref->numrows) + tref->numrows, tref->numrows);
		unsigned int coord = x + y*tref->numcols;
		assert(coord >= 0);
		assert(coord < tref->numrows*tref->numcols);
		return &(tref->normals[coord]);
	}
	else {
		tref = t;
		return t->normals;
	}
}

struct raindrop {
	VEC3 pos;
	VEC3 vel;
	float load;
	int steps;
};

#define DRAG_COEFFICIENT 0.8
#define MAX_RAINDROP_STEPS 1000
#define ACCELERATION_DUE_TO_GRAVITY 9.81 //Units are meters/s^2
#define RAINDROP_MASS 1
#define RAINDROP_FRICTION 0.2
#define RAINDROP_CAPACITY 0.1
#define RAINDROP_SPEEDFLOOR 0.0001
#define FRICTION_COEFFICIENT 0.3

//∆GPE = -∆KE
//mgh = -(1/2)mv^2
//-2gh = v^2
//v = √(-2gh) 

int simulate_raindrop(struct terrain *t, float x, float z)
{
	tpos(t, 0, 0);
	//Raindrop starts at x, y.
	struct raindrop r = {.pos = {{x, 0, z}}, .vel = {{0, 0, 0}}, .load = 0, .steps = 0};
	//r.vel = vec3_normalize((VEC3){{rand_float(), rand_float(), rand_float()}});
	float deltah = 1;
	float timescale = 1;
	int hit_steplimit = 0;
	do {
		VEC3 *p  = tpos(NULL, r.pos.x,   r.pos.z);
		VEC3 *c1 = tpos(NULL, r.pos.x+1, r.pos.z);
		VEC3 *c2 = tpos(NULL, r.pos.x+1, r.pos.z+1);
		VEC3 *c3 = tpos(NULL, r.pos.x,   r.pos.z+1);
		VEC3 grad = vec3_normalize_safe((VEC3){{
			p->y + c3->y - c1->y - c2->y,
			0,
			c2->y + c3->y - p->y - c1->y
		}});

		//Force of gravity pulls down, some becomes normal force, some becomes a force along the slope.
		//If the downward velocity was higher than would keep it on the surface of the next slope segment,
		//calculate the deceleration needed to keep it on the slope, and add it to the normal force.
		//Add up all the forces that acted on the drop in the last segment, apply them, and determine the new
		//direction of travel by normalizing the velocity vector.
		float Fg = ACCELERATION_DUE_TO_GRAVITY * RAINDROP_MASS;
		float b = sqrt(1 + deltah*deltah);
		float Fn = Fg / b;
		float Af = -Fn * (FRICTION_COEFFICIENT / RAINDROP_MASS); //Acceleration (deceleration) due to friction.
		float Ag = deltah / b; //Acceleration due to gravity aligned down the gradient.
		float A = Ag + Af; //Total acceleration experienced by the drop.
		VEC3 a_vec = {{
			grad.x*A,
			grad.y*A,
			grad.z*A
		}};
		//r.vel.x = (r.vel.x + grad.x*deltah*ACCELERATION_DUE_TO_GRAVITY) * DRAG_COEFFICIENT;
		//r.vel.z = (r.vel.z + grad.z*deltah*ACCELERATION_DUE_TO_GRAVITY) * DRAG_COEFFICIENT;
		r.vel = vec3_add(r.vel, vec3_scale(a_vec, timescale));
		timescale = 1/vec3_mag(r.vel);
		assert(timescale != NAN);
		r.pos = vec3_add(r.pos, vec3_scale(r.vel, timescale)); //Move the raindrop by one unit in its velocity direction.
		// r.pos  = vec3_add(r.pos, grad); //Move the raindrop along the gradient.
		VEC3 *newp = tpos(NULL, r.pos.x, r.pos.z);
		deltah = newp->y - p->y;
		if (deltah < 0) { //The drop moves downhill
			float sediment = fmin(RAINDROP_CAPACITY - r.load, -deltah);
			p->y -= sediment;
			r.load += sediment;
		} else {
			float sediment = fmin(r.load, deltah);
			p->y += sediment;
			r.load -= sediment;
		}
		r.steps++;
		if (r.steps >= MAX_RAINDROP_STEPS) {
			hit_steplimit = 1;
			break;
		}
	} while (vec3_mag(r.vel) > RAINDROP_SPEEDFLOOR);
	VEC3 *finalp = tpos(NULL, r.pos.x, r.pos.y);
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

void recalculate_terrain_normals(struct terrain *t)
{
	tpos(t, 0, 0);
	tnorm(t, 0, 0);
	for (int x = 0; x < t->numcols; x++) {
		for (int z = 0; z < t->numrows; z++) {
			// float h[9];
			// for (int j = -1; j < 2; j++) {
			// 	for (int k = -1; k < 2; k++) {
			// 		h[j + k*3] = tpos(NULL, x+k, z+j)->y;
			// 	}
			// }
			// float scale = 5;
			// *tnorm(NULL, x, z) = vec3_normalize((VEC3){{
			// 	scale * -(h[2]-h[0]+2*(h[5]-h[3])+h[8]-h[6]),
			// 	1.0,
			// 	scale * -(h[6]-h[0]+2*(h[7]-h[1])+h[8]-h[2])
			// }});
			VEC3 *p0 = tpos(NULL, x,   z);
			VEC3 *p1 = tpos(NULL, x+1, z);
			VEC3 *p2 = tpos(NULL, x+1, z+1);
			VEC3 *p3 = tpos(NULL, x,   z+1);
			VEC3 *p4 = tpos(NULL, x-1, z+1);
			VEC3 *p5 = tpos(NULL, x-1, z);
			VEC3 *p6 = tpos(NULL, x-1, z-1);
			VEC3 *p7 = tpos(NULL, x, z-1);
			VEC3 *p8 = tpos(NULL, x+1, z-1);
			*tnorm(NULL, x, z) = vec3_normalize(
				vec3_add(
					vec3_add(
						vec3_cross(vec3_sub(*p1, *p0), vec3_sub(*p3, *p0)),
						vec3_cross(vec3_sub(*p5, *p0), vec3_sub(*p7, *p0))
					),
					vec3_add(
						vec3_cross(vec3_sub(*p2, *p0), vec3_sub(*p4, *p0)),
						vec3_cross(vec3_sub(*p6, *p0), vec3_sub(*p8, *p0))
					)
				)
			);
		}
	}
}

void erode_terrain(struct terrain *t, int iterations)
{
	int steplimit_drops = 0;
	for (int i = 0; i < iterations; i++)
		steplimit_drops += simulate_raindrop(t, rand_float()*(t->numcols-1), rand_float()*(t->numrows-1));
	printf("%i drops hit the steplimit this iteration.\n", steplimit_drops);
}

void populate_terrain(struct terrain *t, VEC3 (*height_map)(float x, float y), VEC3 (*height_map_normal)(float x, float y))
{
	//Generate vertices.
	VEC3 color = {{0.7, 0.65, 0.65}};
	for (int i = 0; i < t->numrows; i++) {
		for (int j = 0; j < t->numcols; j++) {
			int offset = (t->numcols * i) + j;
			VEC3 pos = height_map(i, j);
			VEC3 norm = height_map_normal(i, j);
			t->positions[offset] = pos;
			t->normals[offset] = norm;
			t->colors[offset] = color;
		}
	}
}

void buffer_terrain(struct terrain *t)
{
	assert(gpositions == t->positions);
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
