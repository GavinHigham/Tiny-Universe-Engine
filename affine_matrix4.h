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

AM4 AM4_mult_b(AM4 a, AM4 b);
AM4 AM4_mult(AM4 a, AM4 b);
AM4 AM4_rot(AM4 a, float ux, float uy, float uz, float angle);
AM4 AM4_trans(AM4 a, float x, float y, float z);
void AM4_to_array(float *buf, int len, AM4 a);
AM4 AM4_rotmat(float ux, float uy, float uz, float angle);
AM4 AM4_rotmat_lomult(float ux, float uy, float uz, float angle);
AM4 AM4_lookat(V3 p, V3 q, V3 u);
AM4 AM4_inverse(AM4 a);
//AM4 AM4_rotinv(AM4 a);
void AM4_print(AM4 a);

#endif