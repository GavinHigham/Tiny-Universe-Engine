#ifndef SPACE_SECTOR_H
#define SPACE_SECTOR_H

/*
bpos is a "big position". Together with a vec3 position, it can represent a location within a massive
volume, to relatively high precision. 
*/

#include "inttypes.h"
#include "glla.h"

//2^13 meters. Distance from the center of the cell to the edge along any axis.
extern const float BPOS_CELL_SIZE;

typedef qvec3 bpos_origin;
typedef struct {
	vec3 offset;
	qvec3 origin;
} bpos;

//If b's position is outside the bounds of b's origin, corrects the origin and remaps b's position.
void bpos_fix(bpos *b);
//If offset is outside the bounds of origin, corrects origin and remaps offset to it.
void bpos_split_fix(vec3 *offset, qvec3 *origin);
//Returns pos.offset relative to new_origin.
vec3 bpos_remap(bpos pos, qvec3 new_origin);

vec3 bpos_disp(bpos_origin from, bpos_origin to);

//Prints a bpos like so: "[x, y, z]" (no newline).
void bpos_print(bpos b);
//Prints a bpos like so: "[x, y, z]" (no newline). Takes a printf format for printing each int64.
void bpos_printf(char *ifmt, char *ffmt, bpos b);

#endif