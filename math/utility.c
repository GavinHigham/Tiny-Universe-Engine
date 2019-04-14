#include "utility.h"
#include "glla.h"
#include "graphics.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>

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

float fclamp(float value, float min, float max)
{
	return fmin(fmax(value, min), max);
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

lldiv_t lldiv_floor(int64_t a, int64_t b)
{
	lldiv_t d = lldiv(a, b);
	//1, if the remainder is nonzero and different in sign from the denominator
	int64_t corr = (d.rem != 0 && ((d.rem < 0) != (b < 0)));
	d.quot -= corr; //Correct the quotient for floor division.
	d.rem += corr * b; //Correct the remainder for floor division.
	return d;
}

void make_projection_matrix(float fov, float a, float n, float f, float *buf)
{
	float nn = 1.0/tan(fov/2.0);
	float tmp[] = (float []){
		nn, 0,           0,              0,
		0, nn,           0,              0,
		0,  0,   (n)/(n-f),  (n*f)/(n-f),
		//0,  0, (f+n)/(f-n), (-2*f*n)/(f-n),
		0,  0,          -1,              1
	};
	if (a >= 0)
		tmp[0] /= a;
	else
		tmp[5] *= a;
	memcpy(buf, tmp, sizeof(tmp));
}

uint32_t float3_hash(float *f, int precision)
{
	float sum = 0;
	float primes[] = {5, 19, 37, 53};
	float fprecision = pow(2, precision);
	for (int i = 0; i < 3; i++)
		sum = ((int32_t)(sum * fprecision) / fprecision) * primes[i] + f[i];
	return sum;
}

//Keeping these here in case I need them for debugging in the future.
vec3 color_from_position(vec3 position, float scale)
{
	position *= scale;
	float offset = (2/3) * M_PI;
	return (vec3){
		255*(sin(position.x)+1)/2,
		255*(sin(position.y + offset)+1)/2,
		255*(sin(position.z + 2*offset)+1)/2
	};
}

//Keeping these here in case I need them for debugging in the future.
vec3 color_from_float3(float *f, float scale)
{
	return color_from_position((vec3){f[0], f[1], f[2]}, scale);
}

int checkErrors(char *label)
{
	int error = glGetError();
	if (error != GL_NO_ERROR) {
		printf("%s: %d\n", label, error);
	}
	return error;
}

extern SDL_Renderer *renderer;
SDL_Texture * load_texture(char *image_path) {
	SDL_Surface *loaded_surface = NULL;
	SDL_Texture *loaded_texture = NULL;

	loaded_surface = IMG_Load(image_path);
	if (loaded_surface == NULL) {
		printf("Unable to load image %s! SDL_image Error: %s\n", image_path, IMG_GetError());
		return NULL;
	}
	
	loaded_texture = SDL_CreateTextureFromSurface(renderer, loaded_surface);
	if (loaded_texture == NULL) {
		printf("Unable to create texture from %s! SDL Error: %s\n", image_path, SDL_GetError());
	}

	SDL_FreeSurface(loaded_surface);
	return loaded_texture;
}

GLuint load_gl_texture(char *path)
{
	GLuint texture = 0;
	SDL_Surface *surface = IMG_Load(path);
	GLenum texture_format;
	if (!surface) {
		printf("Texture %s could not be loaded.\n", path);
		return 0;
	}

	// Get the number of channels in the SDL surface.
	int num_colors = surface->format->BytesPerPixel;
	bool rgb = surface->format->Rmask == 0x000000ff;
	if (num_colors == 4) {
		texture_format = rgb ? GL_RGBA : GL_BGRA;
	} else if (num_colors == 3) {
		texture_format = rgb ? GL_RGB : GL_BGR;
	} else {
		printf("Image does not have at least 3 color channels.\n");
		goto error;
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SDL_LockSurface(surface);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, surface->w, surface->h, 0, texture_format, GL_UNSIGNED_BYTE, surface->pixels);
	SDL_UnlockSurface(surface);

error:
	SDL_FreeSurface(surface);
	return texture;
}