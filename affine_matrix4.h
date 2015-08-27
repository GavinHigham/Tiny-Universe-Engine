#ifndef AFFINE_MATRIX4_H
#define AFFINE_MATRIX4_H

typedef struct am4 {
	float A[9];
	float T[3];
	char type;
} AM4;

#define AM4_IDENT {{1, 0, 0,  0, 1, 0,  0, 0, 1}}

AM4 AM4_mult_b(AM4 a, AM4 b);
AM4 AM4_mult(AM4 a, AM4 b);
AM4 AM4_rot(AM4 a, float ux, float uy, float uz, float angle);
AM4 AM4_trans(AM4 a, float x, float y, float z);
void AM4_to_array(float *buf, int len, AM4 a);
AM4 AM4_rotmat(float ux, float uy, float uz, float angle);
AM4 AM4_rotmat_lomult(float ux, float uy, float uz, float angle);
//AM4 AM4_rotinv(AM4 a);
void AM4_print(AM4 a);

#endif