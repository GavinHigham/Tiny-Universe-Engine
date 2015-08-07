#ifndef AFFINE_MATRIX4_H
#define AFFINE_MATRIX4_H

typedef struct am4 {
	float A[9];
	float T[3];
	char type;
} AM4;

AM4 apply_withbranch(AM4 a, AM4 b);
AM4 apply(AM4 a, AM4 b);
AM4 rot(AM4 a, float ux, float uy, float uz, float angle);
AM4 trans(AM4 a, float x, float y, float z);
void buffer_AM4(float *buf, int len, AM4 a);
AM4 rotAM4(float ux, float uy, float uz, float angle);
AM4 rotAM4_lomult(float ux, float uy, float uz, float angle);
void printAM4(AM4 a);

#endif