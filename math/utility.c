#include <stdlib.h>
#include <math.h>
#include "utility.h"
#include "glla.h"

#define RANDOM_SEED 42 * 1337 + 0xBAE + 'G'+'r'+'e'+'e'+'n' //An excellent random seed
static int seed = RANDOM_SEED;

void srand_float(int seed)
{
	seed = RANDOM_SEED * seed;
}

float rand_float()
{
	return frand(&seed);
}

//Returns a random float between -1 and 1, taken from http://iquilezles.org/www/articles/sfrand/sfrand.htm
float sfrand(int *seed)
{
	float res;
	seed[0] *= 16807;
	*((unsigned int *) &res) = ( ((unsigned int)seed[0])>>9 ) | 0x40000000;
	return( res-3.0f );
}

//Returns a random float between 0 and 1, taken from http://iquilezles.org/www/articles/sfrand/sfrand.htm
float frand(int *seed)
{
	union {
		float f;
		unsigned int i;
	} res;

	seed[0] *= 16807;

	res.i = ((((unsigned int)seed[0])>>9 ) | 0x3f800000);
	return res.f - 1.0f;
}

vec3 rand_bunched_point3d_in_sphere(vec3 origin, float radius)
{
	radius = pow(rand_float(), 4) * radius; //Distribute stars within the sphere, not on the outside.
	float a1 = rand_float() * 2 * M_PI;
	float a2 = rand_float() * 2 * M_PI;
	return origin + (vec3){radius*sin(a1)*cos(a2), radius*sin(a1)*sin(a2), radius*cos(a1)};
}

vec3 rand_box_fvec3(vec3 corner1, vec3 corner2)
{
	return (vec3){rand_float(), rand_float(), rand_float()} * (corner2 - corner1) + corner1;
}

qvec3 rand_box_qvec3(qvec3 corner1, qvec3 corner2)
{
	return (qvec3){rand(), rand(), rand()} * (corner2 - corner1)/RAND_MAX + corner1;
}

ivec3 rand_box_ivec3(ivec3 corner1, ivec3 corner2)
{
	return (ivec3){rand(), rand(), rand()} * (corner2 - corner1)/RAND_MAX + corner1;
}

svec3 rand_box_svec3(svec3 corner1, svec3 corner2)
{
	qvec3 v = rand_box_qvec3((qvec3){corner1.x, corner1.y, corner1.z}, (qvec3){corner2.x, corner2.y, corner2.z});
	return (svec3){v.x, v.y, v.z};
}

float distance_to_horizon(float R, float h)
{
	return sqrt(2*R*h + h*h);
}

int hash_qvec3(qvec3 v)
{
	return ((v.x * 13 + v.y) * 7 + v.z) * 53 + RANDOM_SEED + 2;
}

int64_t qround_to_multiple(int64_t num, int64_t divisor)
{
	return ((int64_t)round(num/divisor))*divisor;
}

void rgb_to_cmyk(vec3 rgb, vec3 *cmy, float *k)
{
	*cmy = 255 - rgb;
	*k = fmin(fmin(cmy->x, cmy->y), cmy->z);
	*cmy = (*cmy - *k) / (255 - *k);
	*k /= 255;
}

void cmyk_to_rgb(vec3 cmy, float k, vec3 *rgb)
{
	*rgb = cmy*(1-k) + k;
	*rgb = (1-*rgb)*255 + 0.5;
	for (int i = 0; i < 3; i++)
		(*rgb)[i] = round((*rgb)[i]);
}

vec3 rgb_mix_in_cmyk(vec3 a, vec3 b, float alpha)
{
	vec3 a_cmy, b_cmy, c_cmy, c_rgb;
	float a_k, b_k, c_k;
	rgb_to_cmyk(a, &a_cmy, &a_k);
	rgb_to_cmyk(b, &b_cmy, &b_k);
	c_cmy = vec3_lerp(a, b, alpha);
	c_k = a_k * (1-alpha) + b_k * alpha;
	cmyk_to_rgb(c_cmy, c_k, &c_rgb);
	return c_rgb;
}

// Doesn't handle alphas
// vec3 rgb_mix_many_in_cmyk(vec3 *cols, float *alphas, int num)
// {
// 	vec3 cmy, sum_cmy, rgb;
// 	float k, sum_k;
// 	for (int i = 0; i < num; i++) {
// 		rgb_to_cmyk(cols[i], &cmy, &k);
// 		sum_cmy += cmy; sum_k += k;
// 	}
// 	cmyk_to_rgb(sum_cmy/num, sum_k/num, &rgb);
// 	return rgb;
// }

void int_swap(int *a, int *b)
{
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

void int_shuffle(int ints[], int num)
{
	for (int i = num-1; i > 0; i--)
		int_swap(&ints[rand()%(i+1)], &ints[i]);
}
