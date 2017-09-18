#ifndef ELEMENT_H
#define ELEMENT_H
#include <stdlib.h>
#include <lua.h>
#include "glla.h"
#include "macros.h"

/*
Elements make up the surface of a planet. Different elements have different colors and texture. Elements may affect the 
foliage that can grow on a particular part of a planet's surface. They may also affect the shape of mountains or other 
surface features.

In the future they may affect gas clouds, asteroids, stars, etc.
*/

enum element_name {
	MAX_NUM_ELEMENTS = 64 //
};
extern size_t num_elements;

struct element_properties {
	vec3 color; //Expressed 0-255.
	//Additional properties to be defined.
} extern all_element_properties[];

//Returns the element_properties of the ith element.
struct element_properties element_get_properties(int i);

//Returns a subset of element indices, num in length.
void element_get_random_set(int *elements, int num);

#endif