#include <stdlib.h>
#include <math.h>
#include "utility.h"
#include "glla.h"

#define RANDOM_SEED 42 * 1337 + 0xBAE //An excellent random seed

float rand_float()
{
	static int seed = RANDOM_SEED;
	return frand(&seed);
}

//Returns a random float between -1 and 1, taken from http://iquilezles.org/www/articles/sfrand/sfrand.htm
float sfrand(int *seed)
{
	float res;
	seed[0] *= 16807;
	*((unsigned int *) &res) = ( ((unsigned int)seed[0])>>9 ) | 0x40000000;
	return( res-3.0f );
}

//Returns a random float between 0 and 1, taken from http://iquilezles.org/www/articles/sfrand/sfrand.htm
float frand(int *seed)
{
	union {
		float f;
		unsigned int i;
	} res;

	seed[0] *= 16807;

	res.i = ((((unsigned int)seed[0])>>9 ) | 0x3f800000);
	return res.f - 1.0f;
}

vec3 rand_bunched_point3d_in_sphere(vec3 origin, float radius)
{
	radius = pow(rand_float(), 4) * radius; //Distribute stars within the sphere, not on the outside.
	float a1 = rand_float() * 2 * M_PI;
	float a2 = rand_float() * 2 * M_PI;
	return origin + (vec3){radius*sin(a1)*cos(a2), radius*sin(a1)*sin(a2), radius*cos(a1)};
}

vec3 rand_box_point3d(vec3 corner1, vec3 corner2)
{
	return (vec3){rand_float(), rand_float(), rand_float()} * (corner2 - corner1) + corner1;
}
