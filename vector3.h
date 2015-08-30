#ifndef VECTOR3_H
#define VECTOR3_H

typedef struct v3 {
	float A[3];
} V3;

V3 v3_add(V3 a, V3 b);
V3 v3_sub(V3 a, V3 b);
V3 v3_neg(V3 a);
float v3_dot(V3 a, V3 b);

#endif