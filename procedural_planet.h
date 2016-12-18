#ifndef PROCEDURAL_PLANET_H
#define PROCEDURAL_PLANET_H
#include <glalgebra.h>
#include "terrain_types.h"

proc_planet * new_proc_planet(vec3 pos, float radius, height_map_func height);
void free_proc_planet(proc_planet *p);
void proc_planet_drawlist(proc_planet *p, DRAWLIST *terrain_list, vec3 camera_position);

#endif
