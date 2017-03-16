#include <stdio.h>
#include <math.h>
#include <string.h>
#include "space_sector.h"
#include "glla.h"

const float SPACE_SECTOR_SIZE = 8192;

void space_sector_canonicalize(vec3 *pos, space_sector *sec)
{
	for (int i = 0; i < 3; i ++) {
		(*sec)[i] += truncf((*pos)[i] / SPACE_SECTOR_SIZE);
		(*pos)[i] = fmod((*pos)[i], SPACE_SECTOR_SIZE);
		int_fast64_t tmp = truncf((*pos)[i] / (SPACE_SECTOR_SIZE/2));
		(*pos)[i] -= tmp * SPACE_SECTOR_SIZE;
		(*sec)[i] += tmp;
	}
}

//Also copied to stars.vs, so update there too if changed.
vec3 space_sector_position_relative_to_sector(vec3 pos, space_sector sec, space_sector new_sec)
{
	space_sector tmp = sec - new_sec;
	return SPACE_SECTOR_SIZE * (vec3){tmp.x, tmp.y, tmp.z} + pos;
}

void space_sector_print(space_sector sec)
{
	printf("[%lli, %lli, %lli]", sec.x, sec.y, sec.z);
}

void space_sector_printf(char *fmt, space_sector sec)
{
	char format[strlen(fmt) * 3 + 7]; //2 braces + 2 commas + 2 spaces + 1 newline = 7
	sprintf(format, "{%s, %s, %s}", fmt, fmt, fmt);
	printf(format, sec.x, sec.y, sec.z);
}
