#include <math.h>
#include <stdio.h>
#include <assert.h>
#include "glla.h"
#include "math/utility.h"
#include "procedural_terrain.h"

//Todo, make this not rely on a grid pattern. Gather a list of neighbors. Then I can erode arbitrary meshes.

const float ACCELERATION_DUE_TO_GRAVITY = 9.81;

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

vec3 calc_gradient(struct terrain *t, vec3 rpos)
{
		vec3 *p  = tpos(t, rpos.x,   rpos.z);
		vec3 *c1 = tpos(t, rpos.x+1, rpos.z);
		vec3 *c2 = tpos(t, rpos.x+1, rpos.z+1);
		vec3 *c3 = tpos(t, rpos.x,   rpos.z+1);
		vec3 grad = (vec3){
			p->y + c3->y - c1->y - c2->y,
			0,
			c2->y + c3->y - p->y - c1->y
		};
		if (vec3_mag(grad) > 0)
			grad = vec3_normalize(grad);
		return grad;
}

int simulate_raindrop(struct terrain *t, struct raindrop_config rc, float x, float z)
{
	tpos(t, 0, 0);
	//Raindrop starts at x, y.
	struct raindrop r = {.pos = {x, 0, z}, .vel = {0, 0, 0}, .load = 0, .steps = 0};
	float deltah = 1;
	float timescale = 1;
	int hit_steplimit = 0;
	do {
		vec3 *p = tpos(t, r.pos.x, r.pos.z);
		vec3 grad = calc_gradient(t, r.pos);

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
		vec3 a_vec = {
			grad.x*A,
			grad.y*A,
			grad.z*A
		};
		r.vel = r.vel + a_vec * timescale;
		float vmag = vec3_mag(r.vel);
		if (vmag == 0)
			timescale = 1;
		else
			timescale = 1/vmag;
		assert(timescale != NAN);
		r.pos = r.pos + r.vel * timescale; //Move the raindrop by one unit in its velocity direction.
		// r.pos  = r.pos + grad; //Move the raindrop along the gradient.
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
				vec3_cross(*p1 - p0, *p3 - p0) +
				vec3_cross(*p5 - p0, *p7 - p0) +
				vec3_cross(*p2 - p0, *p4 - p0) +
				vec3_cross(*p6 - p0, *p8 - p0)
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

			*tnorm(t, x, z) = vec3_normalize(vec3_cross(*p1 - *p5, *p3 - *p7));
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
