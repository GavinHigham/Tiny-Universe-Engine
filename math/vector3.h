#ifndef VECTOR3_H
#define VECTOR3_H

//Typedef as a union later!
typedef union vec3 {
	float A[3];
	struct {
		float x, y, z;
	};
	struct {
		float r, g, b;
	};
} vec3;

#define vec3_UP {{0, 0, 1}}
#define vec3_ZERO {{0, 0, 0}}
vec3 vec3_zero;

//Produces a new vector. Equivalent to (vec3){{{x, y, z}}}.
vec3 vec3_new(float x, float y, float z);
//Returns a new vector that represents the addition of respective components of a and b.
//(a+b)
vec3 vec3_add(vec3 a, vec3 b);
//Returns a new vector that represents the scaling of a by factor b.
vec3 vec3_scale(vec3 a, float b);
//Returns a new vector that represents the subtraction of respective components of a and b.
//(a - b)
vec3 vec3_sub(vec3 a, vec3 b);
//Returns a new vector that represents the negation of each component of a.
//(-a)
vec3 vec3_neg(vec3 a);
//Returns a new vector that represents the normalization of a.
vec3 vec3_normalize(vec3 a);
//Returns a new vector that represents the normalization of a. Correctly handles zero-vectors, but slower.
vec3 vec3_normalize_safe(vec3 a);
//Returns a new vector that represents the cross product of a and b.
vec3 vec3_cross(vec3 a, vec3 b);
//Returns a new vector that is linearly interpolated between a and by by parameter alpha.
vec3 vec3_lerp(vec3 a, vec3 b, float alpha);
//Return the dot product of a and b.
float vec3_dot(vec3 a, vec3 b);
//Return the magnitude of a.
float vec3_mag(vec3 a);

#endif