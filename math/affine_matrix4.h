#ifndef AFFINE_MATRIX4_H
#define AFFINE_MATRIX4_H
#include "vector3.h"
#include "matrix3.h"

typedef struct AMAT4 {
	union {
		float A[9];
		MAT3 a;
	};
	union {
		float T[3];
		struct {
			float x, y, z;
		};
		VEC3 t;
	};
	char type;
} AMAT4;

#define AMAT4_IDENT {.a = MAT3_IDENT, .t = VEC3_ZERO}
//Multiply a by b and return the result as a new affine matrix.
//This version has branches to try to reduce multiplications. (Faster on ARM?)
AMAT4 amat4_mult_b(AMAT4 a, AMAT4 b);
//Multiply a by b and return the result as a new affine matrix.
AMAT4 amat4_mult(AMAT4 a, AMAT4 b);
//Multiply a by b as a column vector and return a new vector.
//b is implied to be a 4-vec with the form <x, y, z, 1>
VEC3 amat4_multpoint(AMAT4 a, VEC3 b);
//Multiply a by b as a column vector and return a new vector.
//b is implied to be a 4-vec with the form <x, y, z, 0>
//It's probably faster to do mat3_multvec(a.a, b), since that copies less.
VEC3 amat4_multvec(AMAT4 a, VEC3 b);
//Produce a new matrix that is the result of rotating a about the axis <ux, uy, uz> by angle, in radians.
//<ux, uy, uz> should be normalized beforehand.
AMAT4 amat4_rot(AMAT4 a, float ux, float uy, float uz, float angle);
//Translate an affine matrix by <x, y, z>. Simply updates the fourth column.
AMAT4 amat4_trans(AMAT4 a, float x, float y, float z);
//Copy a into a buffer representing a true 4x4 row-major matrix.
//len is the length of the buffer. It should be at least 16.
//The last row will be <0, 0, 0, 1>.
void amat4_to_array(float *buf, int len, AMAT4 a);
//Produce a rotation matrix which represents a rotation about <ux, uy, uz> by angle, in radians.
AMAT4 amat4_rotmat(float ux, float uy, float uz, float angle);
//Produce a rotation matrix which represents a rotation about <ux, uy, uz> by angle, in radians.
//This version tries to reduce multiplications at the cost of more interdependant local variables.
AMAT4 amat4_rotmat_lomult(float ux, float uy, float uz, float angle);
//Produce a lookat matrix that points from point p to point q, with u as "up".
AMAT4 amat4_lookat(VEC3 p, VEC3 q, VEC3 u);
//Produce the inverse matrix to a, provided that a represents a rotation and a translation.
//Does not work if a represents a scale (or a skew?).
AMAT4 amat4_inverse(AMAT4 a);
//AMAT4 amat4_rotinv(AMAT4 a);
void amat4_print(AMAT4 a);

#endif