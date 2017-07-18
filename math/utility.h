#ifndef UTILITY_H
#define UTILITY_H
#include "glla.h"

//Returns a random float between -1 and 1
float rand_float();
//Returns a random float between -1 and 1
float sfrand(int *seed);
//Returns a random float between 0 and 1
float frand(int *seed);
//Produces a random point in a sphere as a vec3.
//Bunched near the middle because I did my math wrong.
//Still looks cool though.
vec3 rand_bunched_point3d_in_sphere(vec3 origin, float radius);
//Produces a random point in a box as a vec3.
vec3 rand_box_fvec3(vec3 corner1, vec3 corner2);
//Produces a random point in a box as a qvec3.
qvec3 rand_box_qvec3(qvec3 corner1, qvec3 corner2);
//Produces a random point in a box as a svec3.
svec3 rand_box_svec3(svec3 corner1, svec3 corner2);
//Generic wrapper that will work with either.
#define rand_box_vec3(corner1, corner2) _Generic((corner1), \
	vec3: rand_box_fvec3, \
	svec3: rand_box_svec3, \
	qvec3: rand_box_qvec3)(corner1, corner2)

#endif