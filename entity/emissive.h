#ifndef EMISSIVE_H
#define EMISSIVE_H
#include "glla.h"
#include <stdbool.h>

typedef struct emissive_component {
	vec3 color;
	vec3 attenuation; //Constant, linear, exponential.
	float radius;
	bool shadowing;
} Emissive;

#endif