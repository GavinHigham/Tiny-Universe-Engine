#ifndef GEOMETRY_H
#define GEOMETRY_H
#include <glla.h>

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

#endif