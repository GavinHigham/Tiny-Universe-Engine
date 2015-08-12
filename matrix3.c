#include <math.h>
#include <string.h>
#include <assert.h>
#include "matrix3.h"

//Create a new mat3 from an array of floats. Row-major order.
MAT3 mat3_from_array(float *array)
{
	MAT3 tmp = {
		{
			array[0], array[1], array[2],
			array[3], array[4], array[5],
			array[6], array[7], array[7]
		}
	};
	return tmp;
}
//Create a new array of floats (suitable for a buffer object) from a mat3. Row-major order.
void mat3_to_array(MAT3 a, float *buf, int len)
{
	assert(len == 9);
	memcpy(buf, a.A, sizeof(a.A));
};
//3x3 Identity matrix.
MAT3 mat3_ident()
{
	MAT3 tmp = {
		{
			1, 0, 0,
			0, 1, 0,
			0, 0, 1
		}
	};
	return tmp;
}
//Multiply a by b
MAT3 mat3_mult(MAT3 a, MAT3 b)
{
	MAT3 tmp = {
		{
			//Top row
			a.A[0] * b.A[0] + a.A[1] * b.A[3] + a.A[2] * b.A[6],
			a.A[0] * b.A[1] + a.A[1] * b.A[4] + a.A[2] * b.A[7],
			a.A[0] * b.A[2] + a.A[1] * b.A[5] + a.A[2] * b.A[8],
			//Second row
			a.A[3] * b.A[0] + a.A[4] * b.A[3] + a.A[5] * b.A[6],
			a.A[3] * b.A[1] + a.A[4] * b.A[4] + a.A[5] * b.A[7],
			a.A[3] * b.A[2] + a.A[4] * b.A[5] + a.A[5] * b.A[8],
			//Third row
			a.A[6] * b.A[0] + a.A[7] * b.A[3] + a.A[8] * b.A[6],
			a.A[6] * b.A[1] + a.A[7] * b.A[4] + a.A[8] * b.A[7],
			a.A[6] * b.A[2] + a.A[7] * b.A[5] + a.A[8] * b.A[8]
		}
	};
	return tmp;
}
//Rotate a about <ux, uy, uz> by "angle" radians. 
MAT3 mat3_rot(MAT3 a, float ux, float uy, float uz, float angle)
{
	return mat3_mult(a, mat3_rotmat(ux, uy, uz, angle));
}
//Rotate a about the three basis vectors by "angle" radians. Slightly more efficient?
MAT3 mat3_rotx(MAT3 a, float angle); //TODO
MAT3 mat3_roty(MAT3 a, float angle); //TODO
MAT3 mat3_rotz(MAT3 a, float angle); //TODO
//Produces a rotation matrix about <ux, uy, uz> by "angle" radians.
MAT3 mat3_rotmat(float ux, float uy, float uz, float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	float c1 = 1-c;
	MAT3 tmp = {
		{
			c + ux*ux*c1, ux*uy*c1 - uz*s, ux*uz*c1 + uy*s, 
			uy*ux*c1 + uz*s, c + uy*uy*c1, uy*uz*c1 - ux*s,
			ux*uz*c1 - uy*s, uy*uz*c1 + ux*s, c + uz*uz*c1
		}
	};
	return tmp;
}
//Produces a rotation matrix about the three basis vectors by "angle" radians. Slightly more efficient?
MAT3 mat3_rotmatx(float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	MAT3 tmp = {
		{
			1, 0, 0,
			0, c, -s,
			0, s, c
		}
	};
	return tmp;
}
MAT3 mat3_rotmaty(float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	MAT3 tmp = {
		{
			c, 0, s,
			0, 1, 0,
			-s, 0, c
		}
	};
	return tmp;
}
MAT3 mat3_rotmatz(float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	MAT3 tmp = {
		{
			c, -s, 0,
			s, c, 0,
			0, 0, 1
		}
	};
	return tmp;
}
//Scale a by x, y, z. IS THIS THE RIGHT WAY, OR IS THE ORDER SWAPPED?
MAT3 mat3_scale(MAT3 a, float x, float y, float z)
{
	a.A[0] *= x;
	a.A[4] *= y;
	a.A[7] *= z;
	return a;
};
//Produce a matrix that will scale by x, y, z.
MAT3 mat3_scalemat(float x, float y, float z)
{
	MAT3 tmp = {
		{
			x, 0, 0,
			0, y, 0,
			0, 0, z
		}
	};
	return tmp;
}
