#include <math.h>
#include "vector3.h"

V3 v3_new(float x, float y, float z)
{
	V3 tmp = {
		.A = {
			x, y, z
		}
	};
	return tmp;
}

V3 v3_add(V3 a, V3 b)
{
	V3 tmp = {
		.A = {
			a.x + b.x,
			a.y + b.y,
			a.z + b.z
		}
	};
	return tmp;
}

V3 v3_sub(V3 a, V3 b)
{
	V3 tmp = {
		.A = {
			a.x - b.x,
			a.y - b.y,
			a.z - b.z
		}
	};
	return tmp;
}

V3 v3_neg(V3 a)
{
	V3 tmp = {
		.A = {
			-a.x,
			-a.y,
			-a.z
		}
	};
	return tmp;
}

V3 v3_normalize(V3 a)
{
	float m = sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
	V3 tmp = {
		.A = {
			a.x/m, a.y/m, a.z/m
		}
	};
	return tmp;
}

V3 v3_cross(V3 u, V3 v)
{
	V3 tmp = {
		.A = {
			u.y*v.z-u.z*v.y,
			u.z*v.x-u.x*v.z,
			u.x*v.y-u.y*v.x
		}
	};
	return tmp;
}

float v3_dot(V3 u, V3 v)
{
	return (
		u.x * v.x +
		u.y * v.y +
		u.z * v.z
	);
}