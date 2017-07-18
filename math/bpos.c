#include <stdio.h>
#include <math.h>
#include <string.h>
#include "bpos.h"

const float BPOS_CELL_SIZE = 8192;

void bpos_fix(bpos *b)
{
	for (int i = 0; i < 3; i++) {
		//If offset is more than a full cell-width away from origin, update origin.
		b->origin[i] += truncf(b->offset[i] / BPOS_CELL_SIZE);
		//Since the origin moved, trim the offset.
		b->offset[i] = fmod(b->offset[i], BPOS_CELL_SIZE);
		//If offset is still more than a half-cell width away from origin, update origin.
		int_fast64_t tmp = truncf(b->offset[i] / (BPOS_CELL_SIZE/2));
		b->origin[i] += tmp;
		//Since origin moved, trim the offset.
		b->offset[i] -= tmp * BPOS_CELL_SIZE;
	}
}

void bpos_split_fix(vec3 *offset, bpos_origin *origin)
{
	bpos tmp = {*offset, *origin};
	bpos_fix(&tmp);
	*offset = tmp.offset;
	*origin = tmp.origin;
}

//Also copied to stars.vs, so update there too if changed.
vec3 bpos_remap(bpos pos, bpos_origin new_origin)
{
	bpos_origin tmp = pos.origin - new_origin;
	return BPOS_CELL_SIZE * (vec3){tmp.x, tmp.y, tmp.z} + pos.offset;
}

vec3 bpos_disp(bpos_origin from, bpos_origin to)
{
	bpos_origin tmp = to - from;
	return BPOS_CELL_SIZE * (vec3){tmp.x, tmp.y, tmp.z};
}

void bpos_print(bpos b)
{
	qvec3_print(b.origin);
	vec3_print(b.offset);
}

void bpos_printf(char *ifmt, char *ffmt, bpos b)
{
	qvec3_printf(ifmt, b.origin);
	vec3_printf(ffmt, b.offset);
}
