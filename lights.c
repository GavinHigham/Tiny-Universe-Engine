#include <math.h>
#include "lights.h"
#include "stdbool.h"
#define BITS_PER_COLOR_CHANNEL 16

float point_light_radius(struct point_light_attributes *lights, int i)
{
	//Most of these should be optimized out by the compiler, right?
	float ci = fmax(fmax(lights->color[i].r, lights->color[i].g), lights->color[i].b); //Color intensity.
	float li = lights->intensity[i];
	float ti = 1.0/pow(2, BITS_PER_COLOR_CHANNEL); //Threshold intensity. We want the radius at which light emitted from our point is this intense.
	float a = lights->atten_e[i];
	float b = lights->atten_l[i];
	float c = lights->atten_c[i] - (ci*li)/ti;

	float d = (-b + sqrtf(b*b - 4*a*c))/(2*a);
	return d;
}

int new_point_light(struct point_light_attributes *lights, VEC3 position, VEC3 color, float atten_c, float atten_l, float atten_e, float intensity)
{
	int num_lights = lights->num_lights;
	if (num_lights == MAX_NUM_LIGHTS)
		return -1;
	lights->position[num_lights] = position;
	lights->color[num_lights] = color;
	lights->atten_c[num_lights] = atten_c;
	lights->atten_l[num_lights] = atten_l;
	lights->atten_e[num_lights] = atten_e;
	lights->intensity[num_lights] = intensity;
	lights->radius[num_lights] = point_light_radius(lights, num_lights);
	lights->enabled_for_draw[num_lights] = true;
	lights->num_lights++;
	return 0;
}