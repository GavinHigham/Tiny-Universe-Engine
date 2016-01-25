#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "affine_matrix4.h"
#include "vector3.h"

#define TRANSLATION 01
#define ROTATION    02
#define SCALE       03

/*
 0  1  2
 3  4  5
 6  7  8
*/

AMAT4 amat4_mult_b(AMAT4 a, AMAT4 b)
{
	AMAT4 tmp = {
		.A = {
			//First row
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
	if (b.type & TRANSLATION) {
		tmp.x = a.A[0] * b.x + a.A[1] * b.y + a.A[2] * b.z + a.x;
		tmp.y = a.A[3] * b.x + a.A[4] * b.y + a.A[5] * b.z + a.y;
		tmp.z = a.A[6] * b.x + a.A[7] * b.y + a.A[8] * b.z + a.z;
	}
	else {
		tmp.x = 0;
		tmp.y = 0;
		tmp.z = 0;
	}
	tmp.type = a.type | b.type;
	return tmp;
}

AMAT4 amat4_mult(AMAT4 a, AMAT4 b)
{
	AMAT4 tmp = {
		.A = {
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
		},
		.T = {
			tmp.x = a.A[0] * b.x + a.A[1] * b.y + a.A[2] * b.z + a.x,
			tmp.y = a.A[3] * b.x + a.A[4] * b.y + a.A[5] * b.z + a.y,
			tmp.z = a.A[6] * b.x + a.A[7] * b.y + a.A[8] * b.z + a.z
		},
		(a.type | b.type)
	};
	return tmp;
}

VEC3 amat4_multpoint(AMAT4 a, VEC3 b)
{
	VEC3 tmp = {{{
		a.A[0]*b.x + a.A[1]*b.y + a.A[2]*b.z + a.x,
		a.A[3]*b.x + a.A[4]*b.y + a.A[5]*b.z + a.y,
		a.A[6]*b.x + a.A[7]*b.y + a.A[8]*b.z + a.z
	}}};
	return tmp;
}

VEC3 amat4_multvec(AMAT4 a, VEC3 b)
{
	VEC3 tmp = {{{
		a.A[0]*b.x + a.A[1]*b.y + a.A[2]*b.z,
		a.A[3]*b.x + a.A[4]*b.y + a.A[5]*b.z,
		a.A[6]*b.x + a.A[7]*b.y + a.A[8]*b.z
	}}};
	return tmp;
}

AMAT4 amat4_rot(AMAT4 a, float ux, float uy, float uz, float angle)
{
	AMAT4 b = amat4_rotmat(ux, uy, uz, angle);
	AMAT4 tmp = {
		.A = {
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
		},
		.T = {a.x, a.y, a.z},
		.type = ROTATION
	};
	return tmp;
}

AMAT4 amat4_trans(AMAT4 a, float x, float y, float z)
{
	a.x += x;
	a.y += y;
	a.z += z;
	return a;
}

void amat4_to_array(float *buf, int len, AMAT4 a)
{
	assert(len == 16);
	float tmp[] = {
		a.A[0], a.A[1], a.A[2], a.x,
		a.A[3], a.A[4], a.A[5], a.y,
		a.A[6], a.A[7], a.A[8], a.z,
		     0,      0,      0,      1
	};
	memcpy(buf, tmp, sizeof(tmp));
};

AMAT4 amat4_rotmat(float ux, float uy, float uz, float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	float c1 = 1-c;
	AMAT4 tmp = {
		.A = {
			c + ux*ux*c1, ux*uy*c1 - uz*s, ux*uz*c1 + uy*s, 
			uy*ux*c1 + uz*s, c + uy*uy*c1, uy*uz*c1 - ux*s,
			ux*uz*c1 - uy*s, uy*uz*c1 + ux*s, c + uz*uz*c1
		},
		.T = {0},
		.type = ROTATION
	};
	return tmp;
}

AMAT4 amat4_rotmat_lomult(float ux, float uy, float uz, float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	float c1 = 1-c;
	float uxc1 = ux*c1;    //Costs one multiply, saves five
	float uyc1 = uy*c1;    //Costs one multiply, saves three
	float uyzc1 = uz*uyc1; //Costs one multiply, saves two
	float uxs = ux * s;    //Costs one multiply, saves two
	float uys = uy * s;    //Costs one multiply, saves two
	float uzs = uz * s;    //Costs one multiply, saves two
	AMAT4 tmp = {
		.A = {
			c + ux*uxc1, uy*uxc1 - uzs, uz*uxc1 + uys, 
			uy*uxc1 + uzs, c + uy*uyc1, uyzc1 - uxs,
			uz*uxc1 - uys, uyzc1 + uxs, c + uz*uz*c1
		},
		.T = {0},
		.type = ROTATION
	};
	return tmp;
}

AMAT4 amat4_lookat(VEC3 p, VEC3 q, VEC3 u)
{
	VEC3 z = vec3_normalize(vec3_sub(q, p));
	VEC3 y = vec3_normalize(u);
	VEC3 x = vec3_cross(y, z);
	AMAT4 tmp = {
		.A = {
			x.x, y.x, z.x,
			x.y, y.y, z.y,
			x.z, y.z, z.z
		},
		.T = {
			p.x, p.y, p.z
		}
	};
	return tmp;
}

AMAT4 amat4_inverse(AMAT4 a)
{
	AMAT4 tmp = {
		.A = {
			a.A[0], a.A[3], a.A[6],
			a.A[1], a.A[4], a.A[7],
			a.A[2], a.A[5], a.A[8]
		},
		.T = {
			(-a.A[0] * a.x) - (a.A[3] * a.y) - (a.A[6] * a.z),
			(-a.A[1] * a.x) - (a.A[4] * a.y) - (a.A[7] * a.z),
			(-a.A[2] * a.x) - (a.A[5] * a.y) - (a.A[8] * a.z)
		}
	};
	return tmp;
}

void amat4_print(AMAT4 a)
{
	printf("%f %f %f\n", a.A[0], a.A[1], a.A[2]);
	printf("%f %f %f\n", a.A[3], a.A[4], a.A[5]);
	printf("%f %f %f\n", a.A[6], a.A[7], a.A[8]);
	printf("\n%f %f %f\n\n", a.x, a.y, a.z);
}