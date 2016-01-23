#include <math.h>
#include "vector3.h"

V3 v3_new(float x, float y, float z)
{
	return (V3){
		.A = {
			x, y, z
		}
	};
}

V3 v3_add(V3 a, V3 b)
{
	return (V3){
		.A = {
			a.x + b.x,
			a.y + b.y,
			a.z + b.z
		}
	};
}

//Returns a new vector that represents the scaling of a by factor b.
V3 v3_scale(V3 a, float b)
{
	return (V3){{{a.x*b, a.y*b, a.z*b}}};
}

V3 v3_sub(V3 a, V3 b)
{
	return (V3){
		.A = {
			a.x - b.x,
			a.y - b.y,
			a.z - b.z
		}
	};
}

V3 v3_neg(V3 a)
{
	return (V3){
		.A = {
			-a.x,
			-a.y,
			-a.z
		}
	};
}

V3 v3_normalize(V3 a)
{
	float m = sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
	return (V3){
		.A = {
			a.x/m, a.y/m, a.z/m
		}
	};
}

V3 v3_cross(V3 u, V3 v)
{
	return (V3){
		.A = {
			u.y*v.z-u.z*v.y,
			u.z*v.x-u.x*v.z,
			u.x*v.y-u.y*v.x
		}
	};
}

float v3_dot(V3 u, V3 v)
{
	return (
		u.x * v.x +
		u.y * v.y +
		u.z * v.z
	);
}