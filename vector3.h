#ifndef VECTOR3_H
#define VECTOR3_H

typedef struct v3 {
	union {
		float A[3];
		struct {
			float x, y, z;
		};
		struct {
			float r, g, b;
		};
	};
} V3;

#define V3_UP {{{0, 0, 1}}}
#define V3_ZERO {{{0, 0, 0}}}

//Produces a new vector. Equivalent to (V3){{{x, y, z}}}.
V3 v3_new(float x, float y, float z);
//Returns a new vector that represents the addition of respective components of a and b.
//(a+b)
V3 v3_add(V3 a, V3 b);
//Returns a new vector that represents the scaling of a by factor b.
V3 v3_scale(V3 a, float b);
//Returns a new vector that represents the subtraction of respective components of a and b.
//(a - b)
V3 v3_sub(V3 a, V3 b);
//Returns a new vector that represents the negation of each component of a.
//(-a)
V3 v3_neg(V3 a);
//Returns a new vector that represents the normalization of a.
V3 v3_normalize(V3 a);
//Returns a new vector that represents the cross product of a and b.
V3 v3_cross(V3 a, V3 b);
//Return the dot product of a and b.
float v3_dot(V3 a, V3 b);

#endif