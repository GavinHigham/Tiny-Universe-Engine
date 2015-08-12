#ifndef MATRIX3_H
#define MATRIX3_H

typedef struct matrix3 {
	float A[9];
} MAT3;

//Create a new mat3 from an array of floats. Row-major order.
MAT3 mat3_from_array(float *array);
//3x3 Identity matrix.
MAT3 mat3_ident();
//Multiply a by b
MAT3 mat3_mult(MAT3 a, MAT3 b);
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

#endif