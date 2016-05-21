#include <math.h>
#include <string.h>
#include "vector3.h"

vec3 vec3_zero = vec3_ZERO;

vec3 vec3_new(float x, float y, float z)
{
	return (vec3){{x, y, z}};
}

vec3 vec3_add(vec3 a, vec3 b)
{
	return (vec3){{a.x + b.x, a.y + b.y, a.z + b.z}};
}

//Returns a new vector that represents the scaling of a by factor b.
vec3 vec3_scale(vec3 a, float b)
{
	return (vec3){{a.x*b, a.y*b, a.z*b}};
}

vec3 vec3_sub(vec3 a, vec3 b)
{
	return (vec3){{a.x - b.x, a.y - b.y, a.z - b.z}};
}

vec3 vec3_neg(vec3 a)
{
	return (vec3){{-a.x, -a.y, -a.z}};
}

vec3 vec3_normalize(vec3 a)
{
	float m = sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
	return (vec3){{a.x/m, a.y/m, a.z/m}};
}

vec3 vec3_normalize_safe(vec3 a)
{
	if (memcmp(&a, &vec3_zero, 3*sizeof(float)) == 0)
		return vec3_zero;
	else
		return vec3_normalize(a);
}

vec3 vec3_cross(vec3 u, vec3 v)
{
	return (vec3){{u.y*v.z-u.z*v.y, u.z*v.x-u.x*v.z, u.x*v.y-u.y*v.x}};
}

vec3 vec3_lerp(vec3 a, vec3 b, float alpha)
{
	float beta = 1 - alpha;
	return (vec3){{a.x * alpha + b.x * beta, a.y * alpha + b.y * beta, a.z * alpha + b.z * beta, }};
}

float vec3_dot(vec3 u, vec3 v)
{
	return (u.x * v.x + u.y * v.y + u.z * v.z);
}

float vec3_mag(vec3 a)
{
	return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
}