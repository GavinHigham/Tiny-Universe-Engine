#ifndef UTILITY_H
#define UTILITY_H
#include <glla.h>

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
vec3 rand_box_point3d(vec3 corner1, vec3 corner2);

#endif