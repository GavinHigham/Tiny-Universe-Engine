#ifndef LIGHTS_H
#define LIGHTS_H
#include "vector3.h"

struct point_light {
	float atten_c; //Constant attenuation.
	float atten_l; //Linear attenuation.
	float atten_e; //Exponential attenuation.
	float intensity; //Light intensity.
	float radius;
	V3 position;
	V3 color;
	int enabled_for_draw; //TRUE or FALSE.
};

float point_light_radius(struct point_light light);

#endif