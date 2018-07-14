#include "solar_system.h"
#include "procedural_planet.h"
#include "../math/bpos.h"
#include "../math/utility.h"
#include "../configuration/lua_configuration.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


/*
Minimum viable prototype notes:
R = 6000000, planet radius between like R * 3 and R / 3
Distance between planets is like R * 50 for now
*/

enum {
	MAX_PLANET_RADIUS = 6000000 * 50, //6000000 is Earth-like
	MIN_PLANET_RADIUS = 6000000 / 50,
	SOLAR_SYSTEM_MAX_RADIUS = 140000, //lol, scientifically chosen I'm sure
};

solar_system solar_system_new(bpos_origin o)
{
	//Determine how many planets there will be
	int seed = hash_qvec3(o);
	srand_float(seed);
	srand(seed);
	solar_system s = {
		.seed = seed,
		.origin = o,
		.num_planets = 1//frand(&seed) * SOLAR_SYSTEM_MAX_PLANETS,
	};
	element_get_random_set(s.elements, SOLAR_SYSTEM_MAX_ELEMENTS);

	//int color_seed = seed;

	//Choose solar system corners
	int64_t w = SOLAR_SYSTEM_MAX_RADIUS;
	qvec3 c1 = {w, w, w}, c2 = {-w, -w, -w};
	_Static_assert(PROC_PLANET_MAX_NUM_ELEMENTS < SOLAR_SYSTEM_MAX_ELEMENTS, "Gavin did some stupid math in solar_system_new");

	printf("Solar system element colors:\n");
	for (int i = 0; i < SOLAR_SYSTEM_MAX_ELEMENTS; i++) {
		printf("[Element %i], ", s.elements[i]); vec3_print(element_get_properties(s.elements[i]).color);
	}
	printf("\n");

	for (int i = 0; i < s.num_planets; i++) {
		printf("Creating a planet! ");
		float radius = (frand(&seed)+1) * (MAX_PLANET_RADIUS-MIN_PLANET_RADIUS) + MIN_PLANET_RADIUS;
		//Is this deterministic?

		int window_start = rand() % (SOLAR_SYSTEM_MAX_ELEMENTS - PROC_PLANET_MAX_NUM_ELEMENTS + 1);
		s.planets[i] = proc_planet_new(radius, proc_planet_height, s.elements + window_start, 2);//PROC_PLANET_MAX_NUM_ELEMENTS);
		qvec3 p;
		//Find a point in the solar system bounding sphere.
		do p = rand_box_qvec3(c1, c2); while (qvec3_sum(p*p) > w*w);
		s.planet_positions[i].origin = p;
		//Later I might use this if I want the planets to actually move slowly following an orbit.
		s.planet_positions[i].offset = (vec3){0,0,0};
		s.planet_positions[i].origin += s.origin;
		qvec3_print(s.planet_positions[i].origin); puts("");
	}

	//Determine the type/chemical makeup of each planet
	//Determine the planet orbits and position along their orbits
	return s;
}

void solar_system_free(solar_system s)
{
	for (int i = 0; i < s.num_planets; i++)
		proc_planet_free(s.planets[i]);
}