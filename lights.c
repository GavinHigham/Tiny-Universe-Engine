#include <math.h>
#include "lights.h"
#include "stdbool.h"

int bits_per_color_channel = 16;

float point_light_radius(struct point_light_attributes *lights, int i)
{
	float ci = fmax(fmax(lights->color[i].x, lights->color[i].y), lights->color[i].z); //Color intensity.
	float li = lights->intensity[i];
	//Threshold intensity. We want the radius at which light emitted from our point is this intense.
	float ti = 1.0/pow(2, bits_per_color_channel);
	float a = lights->atten_e[i];
	float b = lights->atten_l[i];
	float c = lights->atten_c[i] - (ci*li)/ti;

	float d = (-b + sqrtf(b*b - 4*a*c))/(2*a);
	return d;
}

int new_point_light(struct point_light_attributes *lights, vec3 position, vec3 color, float atten_c, float atten_l, float atten_e, float intensity)
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
	lights->shadowing[num_lights] = false;
	lights->num_lights++;
	return 0;
}