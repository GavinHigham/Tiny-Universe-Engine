#include <stdio.h>
#include <math.h>
#include <string.h>
#include "space_sector.h"
#include "glla.h"

const float SPACE_SECTOR_SIZE = 8192;

void space_sector_canonicalize(vec3 *pos, space_sector *sec)
{
	vec3 pos_old = *pos;
	space_sector sec_old = *sec;
	space_sector new = *sec;

	new.x += truncf(pos->x / SPACE_SECTOR_SIZE);
	new.y += truncf(pos->y / SPACE_SECTOR_SIZE);
	new.z += truncf(pos->z / SPACE_SECTOR_SIZE);

	pos->x = fmod(pos->x, SPACE_SECTOR_SIZE);
	pos->y = fmod(pos->y, SPACE_SECTOR_SIZE);
	pos->z = fmod(pos->z, SPACE_SECTOR_SIZE);

	int_fast64_t x = truncf(pos->x / (SPACE_SECTOR_SIZE/2));
	int_fast64_t y = truncf(pos->y / (SPACE_SECTOR_SIZE/2));
	int_fast64_t z = truncf(pos->z / (SPACE_SECTOR_SIZE/2));

	pos->x -= x * SPACE_SECTOR_SIZE;
	pos->y -= y * SPACE_SECTOR_SIZE;
	pos->z -= z * SPACE_SECTOR_SIZE;

	new.x += x;
	new.y += y;
	new.z += z;

	//*pos = space_sector_position_relative_to_sector(*pos, *sec, new);
	*sec = new;
}

//Also copied to stars.vs, so update there too if changed.
vec3 space_sector_position_relative_to_sector(vec3 pos, space_sector sec, space_sector new_sec)
{
	//Local position + (sector displacement on each axis) * (size of a sector)
	return SPACE_SECTOR_SIZE * (vec3){
		sec.x - new_sec.x, //lol secx
		sec.y - new_sec.y,
		sec.z - new_sec.z
		} + pos;
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
