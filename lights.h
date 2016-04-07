#ifndef LIGHTS_H
#define LIGHTS_H
#include <stdbool.h>
#include "math/vector3.h"

#define MAX_NUM_LIGHTS 1000

struct point_light_attributes {
	VEC3 position[MAX_NUM_LIGHTS];
	VEC3 color[MAX_NUM_LIGHTS];
	float atten_c[MAX_NUM_LIGHTS]; //Constant attenuation.
	float atten_l[MAX_NUM_LIGHTS]; //Linear attenuation.
	float atten_e[MAX_NUM_LIGHTS]; //Exponontial attenuation.
	float intensity[MAX_NUM_LIGHTS]; //Light intensity.
	float radius[MAX_NUM_LIGHTS];
	bool enabled_for_draw[MAX_NUM_LIGHTS];
	int num_lights; //Number of created lights.
};

float point_light_radius(struct point_light_attributes *lights, int i);
int new_point_light(struct point_light_attributes *lights, VEC3 position, VEC3 color, float atten_c, float atten_l, float atten_e, float intensity);

#endif