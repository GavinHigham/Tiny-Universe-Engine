#include <math.h>
#include <string.h>
#include <assert.h>
#include "matrix3.h"

//Create a new mat3 from an array of floats. Row-major order.
MAT3 mat3_from_array(float *array)
{
	return (MAT3){
		{
			array[0], array[1], array[2],
			array[3], array[4], array[5],
			array[6], array[7], array[7]
		}
	};
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
	return (MAT3){
		{
			1, 0, 0,
			0, 1, 0,
			0, 0, 1
		}
	};
}
//Multiply a by b
MAT3 mat3_mult(MAT3 a, MAT3 b)
{
	return (MAT3){
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
}
VEC3 mat3_multvec(MAT3 a, VEC3 b)
{
	return (VEC3){
		.A = {
			a.A[0]*b.x + a.A[1]*b.y + a.A[2]*b.z,
			a.A[3]*b.x + a.A[4]*b.y + a.A[5]*b.z,
			a.A[6]*b.x + a.A[7]*b.y + a.A[8]*b.z
		}
	};
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
	return (MAT3){
		{
			c + ux*ux*c1, ux*uy*c1 - uz*s, ux*uz*c1 + uy*s, 
			uy*ux*c1 + uz*s, c + uy*uy*c1, uy*uz*c1 - ux*s,
			ux*uz*c1 - uy*s, uy*uz*c1 + ux*s, c + uz*uz*c1
		}
	};
}
//Produces a rotation matrix about the three basis vectors by "angle" radians. Slightly more efficient?
MAT3 mat3_rotmatx(float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	return (MAT3){
		{
			1, 0, 0,
			0, c, -s,
			0, s, c
		}
	};
}
MAT3 mat3_rotmaty(float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	return (MAT3){
		{
			c, 0, s,
			0, 1, 0,
			-s, 0, c
		}
	};
}
MAT3 mat3_rotmatz(float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	return (MAT3){
		{
			c, -s, 0,
			s, c, 0,
			0, 0, 1
		}
	};
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
	return (MAT3){
		{
			x, 0, 0,
			0, y, 0,
			0, 0, z
		}
	};
}

MAT3 mat3_transp(MAT3 a)
{
	return (MAT3){
		{
			a.A[0], a.A[3], a.A[6],
			a.A[1], a.A[4], a.A[7],
			a.A[2], a.A[5], a.A[8]
		}
	};
}

MAT3 mat3_lookat(VEC3 p, VEC3 q, VEC3 u)
{
	VEC3 z = vec3_normalize(vec3_sub(p, q)); //Swapped p and q from the book.
	VEC3 x = vec3_normalize(vec3_cross(u, z));
	VEC3 y = vec3_cross(z, x);
	return (MAT3){{
		x.x, y.x, z.x,
		x.y, y.y, z.y,
		x.z, y.z, z.z
	}};
}

void mat3_vec3_to_array(float *buf, int len, MAT3 a, VEC3 b)
{
	assert(len == 16);
	float tmp[] = {
		a.A[0], a.A[1], a.A[2], b.x,
		a.A[3], a.A[4], a.A[5], b.y,
		a.A[6], a.A[7], a.A[8], b.z,
		     0,      0,      0,    1
	};
	memcpy(buf, tmp, sizeof(tmp));
}
