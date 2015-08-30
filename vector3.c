#include "vector3.h"

V3 v3_add(V3 a, V3 b)
{
	V3 tmp = {
		{
			a.A[0] + b.A[0],
			a.A[1] + b.A[1],
			a.A[2] + b.A[2]
		}
	};
	return tmp;
}

V3 v3_sub(V3 a, V3 b)
{
	V3 tmp = {
		{
			a.A[0] - b.A[0],
			a.A[1] - b.A[1],
			a.A[2] - b.A[2]
		}
	};
	return tmp;
}

V3 v3_neg(V3 a)
{
	V3 tmp = {
		{
			-a.A[0],
			-a.A[1],
			-a.A[2]
		}
	};
	return tmp;
}

float v3_dot(V3 a, V3 b)
{
	return (
		a.A[0] * b.A[0] +
		a.A[1] * b.A[1] +
		a.A[2] * b.A[2]
	);
}