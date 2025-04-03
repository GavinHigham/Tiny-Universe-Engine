#ifndef UTILITY_H
#define UTILITY_H
#include "glla.h"
#include "graphics.h"
#include <stdlib.h>
#include <inttypes.h>
#include <SDL3/SDL.h>

//TODO(Gavin): Move this out of the "math" directory.

// Random numbers

//Seeds the random float number generator.
void srand_float(uint32_t seed);
//Returns a random float between -1 and 1
float rand_float();
//Returns a random float between -1 and 1
float sfrand(uint32_t *seed);
//Returns a random float between 0 and 1
float frand(uint32_t *seed);
//Returns value, clamped between min and max
float fclamp(float value, float min, float max);

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
uint32_t hash_qvec3(qvec3 v);

//A stupid hash for arrays of floats, so I can color them and distinguish them visually.
//Keeping these here in case I need them for debugging in the future.
uint32_t float3_hash(float *f, int precision);

// Geometry
float ico_inscribed_radius(float edge_len);
float ico_circumscribed_radius(float edge_len);

//Distance to the horizon with a planet radius R and elevation above sea level h, from Wikipedia.
float distance_to_horizon(float R, float h);

float fov_to_focal(float fov);

// Color

//Converts an rgb color to a cmyk color.
void rgb_to_cmyk(vec3 a, vec3 *cmy, float *k);
//Converts a cmyk color to an rgb color.
void cmyk_to_rgb(vec3 cmy, float k, vec3 *rgb);
//Mixes two RGB colors a and b in cmyk.
vec3 rgb_mix_in_cmyk(vec3 a, vec3 b, float alpha);
//Google's Turbo colormap translated to C
vec3 turbo_colormap(float x);

// Swapping and shuffling

//Swaps the integers pointed to by a and b.
void int_swap(int *a, int *b);

//An implementation of the Fisher–Yates shuffle that operates on an array of integers.
//Useful for random indices into an array set.
//TODO(Gavin): Add a random seed argument.
void int_shuffle(int ints[], int num);

//Misc. (split this category as it grows)

//Performs a division towards negative infinity.
lldiv_t lldiv_floor(int64_t a, int64_t b);

//Given player position p, slot width, number of circular buffer slots, and slot index,
//Return the index for the slot contents that should be loaded.
int64_t qcircular_buffer_slot(int64_t p, int64_t slot_width, int64_t num_slots, int64_t slot_idx);

//Given player position p, slot width, number of circular buffer slots, and slot index,
//Return the index for the slot contents that should be loaded.
//Does it independently on each axis, implementing a hypertoroidal buffer.
qvec3 qhypertoroidal_buffer_slot(qvec3 p, qvec3 slot_width, qvec3 num_slots, qvec3 slot_idx);

//Create a projection matrix with "fov" field of view, "a" aspect ratio, n and f near and far planes.
//Stick it into buffer buf, ready to send to OpenGL.
void make_projection_matrix(float fov, float a, float n, float f, float *buf);

//Create an orthographic projection matrix and stick it into buffer buf,
//with parameters left, right, bottom, top, near, far.
void make_ortho_matrix(float l, float r, float b, float t, float n, float f, float *buf);

//
int get_tri_lerp_vals(float *lerps, int num_rows);

//Realloc, but sets new memory to 0.
void * crealloc(void *ptr, size_t new_size, size_t old_size);

//Reserve part of a chunk of memory, decrementing "remaining" by size each time.
//Returns NULL if remaining < size, otherwise returns a pointer to the newly reserved memory.
void * alloc_from_chunk(void **chunk, size_t *remaining, size_t size);

SDL_Texture * load_texture(SDL_Renderer *renderer, char *image_path);
GLuint load_gl_texture(char *path);

#endif