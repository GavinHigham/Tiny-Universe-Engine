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

typedef int_fast64_t bpos_origin __attribute__((ext_vector_type(3)));
typedef struct {
	vec3 pos;
	bpos_origin origin;
} bpos;

//If b's position is outside the bounds of b's origin, corrects the origin and remaps b's position.
void bpos_fix(bpos *b);
//If pos is outside the bounds of origin, corrects origin and remaps pos to it.
void bpos_split_fix(vec3 *pos, bpos_origin *origin);
//If pos is expressed relative to bpos_origin, returns pos relative to new_origin.
vec3 bpos_remap(vec3 pos, bpos_origin old_origin, bpos_origin new_origin);

//Prints a bpos like so: "[x, y, z]" (no newline).
void bpos_print(bpos b);
//Prints a bpos like so: "[x, y, z]" (no newline). Takes a printf format for printing each int64.
void bpos_printf(char *ifmt, char *ffmt, bpos b);
//Prints a bpos origin like so: "[x, y, z]" (no newline).
void bpos_origin_print(bpos_origin b);
//Prints a bpos origin like so: "[x, y, z]" (no newline). Takes a printf format for printing each int64.
void bpos_origin_printf(char *fmt, bpos_origin b);

#endif