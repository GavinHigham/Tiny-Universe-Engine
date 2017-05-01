#ifndef GEOMETRY_H
#define GEOMETRY_H
#include "glla.h"
#include <stdbool.h>

/*
Some values will be squared and stored with suffix _sq.
Some values will have their reciprocal taken, and stored with suffix _rec. 
*/

struct sphere {
	vec3 center;
	float radius;
	float radius_sq;
};

struct cone {
	vec3 vertex;
	vec3 axis;
	float sin_sq;
	float cos_sq;
	float sin_rec;
};

bool sphere_cone_intersect(struct sphere s, struct cone c);
int ray_tri_intersect(vec3 r_start, vec3 r_end, vec3 tri[3], vec3 *intersection);
int ray_tri_intersect_with_parametric(vec3 r_start, vec3 r_end, vec3 tri[3], vec3 *intersection, float *tri_s, float *tri_t);

#endif