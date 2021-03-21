#ifndef UNIVERSE_COMPONENTS_H
#define UNIVERSE_COMPONENTS_H

#include "datastructures/ecs.h"
#include "glla.h"
#include "math/bpos.h"
#include "trackball/trackball.h"
#define CD ecs_c_constructor_destructor

typedef struct component_trackball {
	uint32_t target;
	struct trackball trackball;
} Trackball;
CD(trackball_constructor);

#endif