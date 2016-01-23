#ifndef MATRIX3_H
#define MATRIX3_H

#include "vector3.h"

typedef struct matrix3 {
	float A[9];
} MAT3;

#define MAT3_IDENT (MAT3){{1, 0, 0,  0, 1, 0,  0, 0, 1}}

//Create a new mat3 from an array of floats. Row-major order.
MAT3 mat3_from_array(float *array);
//3x3 Identity matrix.
MAT3 mat3_ident();
//Multiply a by b
MAT3 mat3_mult(MAT3 a, MAT3 b);
//Multiply a by the column vector b.
V3 mat3_multvec(MAT3 a, V3 b);
//Rotate a about <ux, uy, uz> by "angle" radians. 
MAT3 mat3_rot(MAT3 a, float ux, float uy, float uz, float angle);
//Rotate a about the three basis vectors by "angle" radians. Slightly more efficient?
MAT3 mat3_rotx(MAT3 a, float angle);
MAT3 mat3_roty(MAT3 a, float angle);
MAT3 mat3_rotz(MAT3 a, float angle);
//Produces a rotation matrix about <ux, uy, uz> by "angle" radians.
MAT3 mat3_rotmat(float ux, float uy, float uz, float angle);
//Produces a rotation matrix about the three basis vectors by "angle" radians. Slightly more efficient?
MAT3 mat3_rotmatx(float angle);
MAT3 mat3_rotmaty(float angle);
MAT3 mat3_rotmatz(float angle);
//Scale a by x, y, z.
MAT3 mat3_scale(MAT3 a, float x, float y, float z);
//Produce a matrix that will scale by x, y, z.
MAT3 mat3_scalemat(float x, float y, float z);
MAT3 mat3_transp(MAT3 a);
//Produce a rotation matrix that will look from p to q, with u up.
MAT3 mat3_lookat(V3 p, V3 q, V3 u);
//Takes a mat3 and a vec3, and copies them into a buffer representing a true, row-major 4x4 matrix.
//a becomes the rotation portion, and b becomes the translation.
void mat3_v3_to_array(float *buf, int len, MAT3 a, V3 b);

#endif