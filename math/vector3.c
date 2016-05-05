#include <math.h>
#include "vector3.h"

VEC3 vec3_new(float x, float y, float z)
{
	return (VEC3){{x, y, z}};
}

VEC3 vec3_add(VEC3 a, VEC3 b)
{
	return (VEC3){{a.x + b.x, a.y + b.y, a.z + b.z}};
}

//Returns a new vector that represents the scaling of a by factor b.
VEC3 vec3_scale(VEC3 a, float b)
{
	return (VEC3){{a.x*b, a.y*b, a.z*b}};
}

VEC3 vec3_sub(VEC3 a, VEC3 b)
{
	return (VEC3){{a.x - b.x, a.y - b.y, a.z - b.z}};
}

VEC3 vec3_neg(VEC3 a)
{
	return (VEC3){{-a.x, -a.y, -a.z}};
}

VEC3 vec3_normalize(VEC3 a)
{
	float m = sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
	return (VEC3){{a.x/m, a.y/m, a.z/m}};
}

VEC3 vec3_cross(VEC3 u, VEC3 v)
{
	return (VEC3){{u.y*v.z-u.z*v.y, u.z*v.x-u.x*v.z, u.x*v.y-u.y*v.x}};
}

VEC3 vec3_lerp(VEC3 a, VEC3 b, float alpha)
{
	float beta = 1 - alpha;
	return (VEC3){{a.x * alpha + b.x * beta, a.y * alpha + b.y * beta, a.z * alpha + b.z * beta, }};
}

float vec3_dot(VEC3 u, VEC3 v)
{
	return (u.x * v.x + u.y * v.y + u.z * v.z);
}