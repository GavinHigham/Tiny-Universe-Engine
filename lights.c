#include <math.h>
#include "lights.h"

float point_light_radius(struct point_light light)
{
	//Most of these should be optimized out by the compiler, right?
	float ci = fmax(fmax(light.color.r, light.color.g), light.color.b); //Color intensity.
	float li = light.intensity;
	float ti = 1.0/256.0; //Threshold intensity. We want the radius at which light emmited from our point is this intense.
	float a = light.atten_e;
	float b = light.atten_l;
	float c = light.atten_c - (ci*li)/ti;

	float d = (-b + sqrtf(b*b - 4*a*c))/(2*a);
	return d;
}