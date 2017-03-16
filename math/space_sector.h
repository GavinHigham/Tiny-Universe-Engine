#ifndef SPACE_SECTOR_H
#define SPACE_SECTOR_H

#include "inttypes.h"
#include "glla.h"

//2^13 meters. Distance from the center of the sector to the edge along any axis.
extern const float SPACE_SECTOR_SIZE;
// typedef struct {
// 	int_fast64_t x, y, z;
// } space_sector;

typedef int_fast64_t space_sector __attribute__((ext_vector_type(3)));

//If pos is outside the bounds of sec, sets sec to the correct sector enclosing pos, and makes pos relative to that origin.
void space_sector_canonicalize(vec3 *pos, space_sector *sec);
//If pos is expressed relative to sec, returns pos relative to new_sec.
vec3 space_sector_position_relative_to_sector(vec3 pos, space_sector sec, space_sector new_sec);

//Prints a space_sector like so: "[x, y, z]" (no newline).
void space_sector_print(space_sector sec);
//Prints a space_sector like so: "[x, y, z]" (no newline). Takes a printf format for printing each float.
void space_sector_printf(char *fmt, space_sector sec);

#endif