#include <stdio.h>
#include <math.h>
#include <string.h>
#include "bpos.h"

const float BPOS_CELL_SIZE = 8192;

void bpos_fix(bpos *b)
{
	for (int i = 0; i < 3; i++) {
		b->origin[i] += truncf(b->pos[i] / BPOS_CELL_SIZE);
		b->pos[i] = fmod(b->pos[i], BPOS_CELL_SIZE);
		int_fast64_t tmp = truncf(b->pos[i] / (BPOS_CELL_SIZE/2));
		b->pos[i] -= tmp * BPOS_CELL_SIZE;
		b->origin[i] += tmp;
	}
}

void bpos_split_fix(vec3 *pos, bpos_origin *origin)
{
	bpos tmp = {*pos, *origin};
	bpos_fix(&tmp);
	*pos = tmp.pos;
	*origin = tmp.origin;
}

//Also copied to stars.vs, so update there too if changed.
vec3 bpos_remap(vec3 pos, bpos_origin old_origin, bpos_origin new_origin)
{
	bpos_origin tmp = old_origin - new_origin;
	return BPOS_CELL_SIZE * (vec3){tmp.x, tmp.y, tmp.z} + pos;
}

void bpos_print(bpos b)
{
	bpos_origin_print(b.origin);
	vec3_print(b.pos);
}

void bpos_printf(char *ifmt, char *ffmt, bpos b)
{
	bpos_origin_printf(ifmt, b.origin);
	vec3_printf(ffmt, b.pos);
}

void bpos_origin_print(bpos_origin b)
{
	printf("[%lli, %lli, %lli]", b.x, b.y, b.z);
}

void bpos_origin_printf(char *fmt, bpos_origin b)
{
	char format[strlen(fmt) * 3 + 7]; //2 braces + 2 commas + 2 spaces + 1 newline = 7
	sprintf(format, "{%s, %s, %s}", fmt, fmt, fmt);
	printf(format, b.x, b.y, b.z);
}
