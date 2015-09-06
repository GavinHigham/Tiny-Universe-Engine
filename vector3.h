#ifndef VECTOR3_H
#define VECTOR3_H

typedef struct v3 {
	union {
		float A[3];
		struct {
			float x, y, z;
		};
	};
} V3;

#define V3_UP {{{0, 0, 1}}}
#define V3_ZERO {{{0, 0, 0}}}

V3 v3_new(float x, float y, float z);
V3 v3_add(V3 a, V3 b);
V3 v3_sub(V3 a, V3 b);
V3 v3_neg(V3 a);
V3 v3_normalize(V3 a);
V3 v3_cross(V3 a, V3 b);
float v3_dot(V3 a, V3 b);

#endif