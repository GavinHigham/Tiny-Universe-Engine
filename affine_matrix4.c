#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "affine_matrix4.h"

#define TRANSLATION 01
#define ROTATION    02
#define SCALE       03

/*
 0  1  2
 3  4  5
 6  7  8
*/

AM4 apply_withbranch(AM4 a, AM4 b)
{
	AM4 tmp = {
		{
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
		tmp.T[0] = a.A[0] * b.T[0] + a.A[1] * b.T[1] + a.A[2] * b.T[2] + a.T[0];
		tmp.T[1] = a.A[3] * b.T[0] + a.A[4] * b.T[1] + a.A[5] * b.T[2] + a.T[1];
		tmp.T[2] = a.A[6] * b.T[0] + a.A[7] * b.T[1] + a.A[8] * b.T[2] + a.T[2];
	}
	else {
		tmp.T[0] = 0;
		tmp.T[1] = 0;
		tmp.T[2] = 0;
	}
	tmp.type = a.type | b.type;
	return tmp;
}

AM4 apply(AM4 a, AM4 b)
{
	AM4 tmp = {
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
		},
		{
			tmp.T[0] = a.A[0] * b.T[0] + a.A[1] * b.T[1] + a.A[2] * b.T[2] + a.T[0],
			tmp.T[1] = a.A[3] * b.T[0] + a.A[4] * b.T[1] + a.A[5] * b.T[2] + a.T[1],
			tmp.T[2] = a.A[6] * b.T[0] + a.A[7] * b.T[1] + a.A[8] * b.T[2] + a.T[2]
		},
		(a.type | b.type)
	};
	return tmp;
}

AM4 rot(AM4 a, float ux, float uy, float uz, float angle)
{
	AM4 b = rotAM4(ux, uy, uz, angle);
	AM4 tmp = {
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
		.T = {a.T[0], a.T[1], a.T[2]},
		.type = ROTATION
	};
	return tmp;
}

AM4 trans(AM4 a, float x, float y, float z)
{
	a.T[0] += x;
	a.T[1] += y;
	a.T[2] += z;
	return a;
}

void buffer_AM4(float *buf, int len, AM4 a)
{
	assert(len == 16);
	float tmp[] = {
		a.A[0], a.A[1], a.A[2], a.T[0],
		a.A[3], a.A[4], a.A[5], a.T[1],
		a.A[6], a.A[7], a.A[8], a.T[2],
		     0,      0,      0,      1
	};
	memcpy(buf, tmp, sizeof(tmp));
};

AM4 rotAM4(float ux, float uy, float uz, float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	float c1 = 1-c;
	AM4 tmp = {
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

AM4 rotAM4_lomult(float ux, float uy, float uz, float angle)
{
	float s = sin(angle);
	float c = cos(angle);
	float c1 = 1-c;
	float uxc1 = ux*c1; //Costs one multiply, saves five
	float uyc1 = uy*c1; //Costs one multiply, saves three
	float uyzc1 = uz*uyc1; //Costs one multiply, saves two
	float uxs = ux * s; //Costs one multiply, saves two
	float uys = uy * s; //Costs one multiply, saves two
	float uzs = uz * s; //Costs one multiply, saves two
	AM4 tmp = {
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

void printAM4(AM4 a)
{
	printf("%f %f %f\n", a.A[0], a.A[1], a.A[2]);
	printf("%f %f %f\n", a.A[3], a.A[4], a.A[5]);
	printf("%f %f %f\n", a.A[6], a.A[7], a.A[8]);
	printf("\n%f %f %f\n\n", a.T[0], a.T[1], a.T[2]);
}