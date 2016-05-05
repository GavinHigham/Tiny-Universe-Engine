#ifndef VECTOR3_H
#define VECTOR3_H

//Typedef as a union later!
typedef union VEC3 {
	float A[3];
	struct {
		float x, y, z;
	};
	struct {
		float r, g, b;
	};
} VEC3;

#define VEC3_UP {{0, 0, 1}}
#define VEC3_ZERO {{0, 0, 0}}

//Produces a new vector. Equivalent to (VEC3){{{x, y, z}}}.
VEC3 vec3_new(float x, float y, float z);
//Returns a new vector that represents the addition of respective components of a and b.
//(a+b)
VEC3 vec3_add(VEC3 a, VEC3 b);
//Returns a new vector that represents the scaling of a by factor b.
VEC3 vec3_scale(VEC3 a, float b);
//Returns a new vector that represents the subtraction of respective components of a and b.
//(a - b)
VEC3 vec3_sub(VEC3 a, VEC3 b);
//Returns a new vector that represents the negation of each component of a.
//(-a)
VEC3 vec3_neg(VEC3 a);
//Returns a new vector that represents the normalization of a.
VEC3 vec3_normalize(VEC3 a);
//Returns a new vector that represents the cross product of a and b.
VEC3 vec3_cross(VEC3 a, VEC3 b);
//Returns a new vector that is linearly interpolated between a and by by parameter alpha.
VEC3 vec3_lerp(VEC3 a, VEC3 b, float alpha);
//Return the dot product of a and b.
float vec3_dot(VEC3 a, VEC3 b);

#endif