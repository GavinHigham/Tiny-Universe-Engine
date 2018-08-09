#ifndef SOLAR_SYSTEM_H
#define SOLAR_SYSTEM_H
#include "math/bpos.h"
#include "procedural_planet.h"
#include "element.h"

/*
A solar system is defined as a star, and some number of planets.
It can include various bits of information that would be helpful for navigating a solar system.
*/

enum {
	SOLAR_SYSTEM_MAX_PLANETS = 10,
	SOLAR_SYSTEM_MAX_ELEMENTS = 10,
};

typedef struct solar_system {
	bpos_origin origin;
	proc_planet *planets[SOLAR_SYSTEM_MAX_PLANETS];
	bpos planet_positions[SOLAR_SYSTEM_MAX_PLANETS];
	int elements[SOLAR_SYSTEM_MAX_ELEMENTS];
	int num_planets;
	int seed;
} solar_system;

solar_system solar_system_new(bpos_origin o);
void solar_system_free(solar_system s);

#endif