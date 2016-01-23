#ifndef AFFINE_MATRIX4_H
#define AFFINE_MATRIX4_H
#include "vector3.h"
#include "matrix3.h"

typedef struct am4 {
	union {
		float A[9];
		MAT3 a;
	};
	union {
		float T[3];
		struct {
			float x, y, z;
		};
		V3 t;
	};
	char type;
} AM4;

#define AM4_IDENT {.a = MAT3_IDENT, .t = V3_ZERO}
//Multiply a by b and return the result as a new affine matrix.
//This version has branches to try to reduce multiplications. (Faster on ARM?)
AM4 AM4_mult_b(AM4 a, AM4 b);
//Multiply a by b and return the result as a new affine matrix.
AM4 AM4_mult(AM4 a, AM4 b);
//Multiply a by b as a column vector and return a new vector.
//b is implied to be a 4-vec with the form <x, y, z, 1>
V3 AM4_multpoint(AM4 a, V3 b);
//Multiply a by b as a column vector and return a new vector.
//b is implied to be a 4-vec with the form <x, y, z, 0>
//It's probably faster to do mat3_multvec(a.a, b), since that copies less.
V3 AM4_multvec(AM4 a, V3 b);
//Produce a new matrix that is the result of rotating a about the axis <ux, uy, uz> by angle, in radians.
//<ux, uy, uz> should be normalized beforehand.
AM4 AM4_rot(AM4 a, float ux, float uy, float uz, float angle);
//Translate an affine matrix by <x, y, z>. Simply updates the fourth column.
AM4 AM4_trans(AM4 a, float x, float y, float z);
//Copy a into a buffer representing a true 4x4 row-major matrix.
//len is the length of the buffer. It should be at least 16.
//The last row will be <0, 0, 0, 1>.
void AM4_to_array(float *buf, int len, AM4 a);
//Produce a rotation matrix which represents a rotation about <ux, uy, uz> by angle, in radians.
AM4 AM4_rotmat(float ux, float uy, float uz, float angle);
//Produce a rotation matrix which represents a rotation about <ux, uy, uz> by angle, in radians.
//This version tries to reduce multiplications at the cost of more interdependant local variables.
AM4 AM4_rotmat_lomult(float ux, float uy, float uz, float angle);
//Produce a lookat matrix that points from point p to point q, with u as "up".
AM4 AM4_lookat(V3 p, V3 q, V3 u);
//Produce the inverse matrix to a, provided that a represents a rotation and a translation.
//Does not work if a represents a scale (or a skew?).
AM4 AM4_inverse(AM4 a);
//AM4 AM4_rotinv(AM4 a);
void AM4_print(AM4 a);

#endif