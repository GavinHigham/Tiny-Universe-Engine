#ifndef UTILITY_H
#define UTILITY_H
#include "glla.h"
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <gl/glew.h>

// printf colors
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_NUMERIC_FCOLOR "\x1b[38;5;%im"
#define ANSI_NUMERIC_BCOLOR "\x1b[48;5;%im"
#define ANSI_FCOLOR "\x1b[38;2;%i;%i;%im"
#define ANSI_BCOLOR "\x1b[48;2;%i;%i;%im"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Random numbers

//Seeds the random float number generator.
void srand_float(int seed);
//Returns a random float between -1 and 1
float rand_float();
//Returns a random float between -1 and 1
float sfrand(int *seed);
//Returns a random float between 0 and 1
float frand(int *seed);

//Distance to the horizon with a planet radius R and elevation above sea level h, from Wikipedia.
float distance_to_horizon(float R, float h);

// Random points

//Produces a random point in a sphere as a vec3.
//Bunched near the middle because I did my math wrong.
//Still looks cool though.
vec3 rand_bunched_point3d_in_sphere(vec3 origin, float radius);
//Produces a random point in a box as a vec3.
vec3 rand_box_fvec3(vec3 corner1, vec3 corner2);
//Produces a random point in a box as a qvec3.
qvec3 rand_box_qvec3(qvec3 corner1, qvec3 corner2);
//Produces a random point in a box as an ivec3.
ivec3 rand_box_ivec3(ivec3 corner1, ivec3 corner2);
//Produces a random point in a box as a svec3.
svec3 rand_box_svec3(svec3 corner1, svec3 corner2);
//Generic wrapper that will work with either.
#define rand_box_vec3(corner1, corner2) _Generic((corner1), \
	vec3: rand_box_fvec3, \
	svec3: rand_box_svec3, \
	ivec3: rand_box_ivec3, \
	qvec3: rand_box_qvec3)(corner1, corner2)

//Returns an int derived from the 3 coordinates of v.
int hash_qvec3(qvec3 v);

// Color

//Converts an rgb color to a cmyk color.
void rgb_to_cmyk(vec3 a, vec3 *cmy, float *k);
//Converts a cmyk color to an rgb color.
void cmyk_to_rgb(vec3 cmy, float k, vec3 *rgb);
//Mixes two RGB colors a and b in cmyk.
vec3 rgb_mix_in_cmyk(vec3 a, vec3 b, float alpha);

// Swapping and shuffling

//Swaps the integers pointed to by a and b.
void int_swap(int *a, int *b);

//An implementation of the Fisher–Yates shuffle that operates on an array of integers.
//Useful for random indices into an array set.
//TODO(Gavin): Add a random seed argument.
void int_shuffle(int ints[], int num);

//Performs a division towards negative infinity.
lldiv_t lldiv_floor(int64_t a, int64_t b);

//Create a projection matrix with "fov" field of view, "a" aspect ratio, n and f near and far planes.
//Stick it into buffer buf, ready to send to OpenGL.
void make_projection_matrix(float fov, float a, float n, float f, float *buf);


int checkErrors(char *label);

SDL_Texture * load_texture(char *image_path);
GLuint load_gl_texture(char *path);

#endif