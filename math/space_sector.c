#include <math.h>
#include "space_sector.h"
#include "glla.h"
#include "stdio.h"

const float SPACE_SECTOR_SIZE = 8192;

void space_sector_canonicalize(vec3 *pos, space_sector *sec)
{
	vec3 pos_copy = *pos;
	sec->x += pos->x / SPACE_SECTOR_SIZE;
	sec->y += pos->y / SPACE_SECTOR_SIZE;
	sec->z += pos->z / SPACE_SECTOR_SIZE;

	pos->x = fmod(pos->x, SPACE_SECTOR_SIZE);
	pos->y = fmod(pos->y, SPACE_SECTOR_SIZE);
	pos->z = fmod(pos->z, SPACE_SECTOR_SIZE);

	if (pos->x != pos_copy.x || pos->y != pos_copy.y || pos->z != pos_copy.z)
		printf("Position renormalized from (%f, %f, %f) to (%f, %f, %f)\n", pos_copy.x, pos_copy.y, pos_copy.z, pos->x, pos->y, pos->z);
}

//Also copied to stars.vs, so update there too if changed.
vec3 space_sector_position_relative_to_sector(vec3 pos, space_sector sec, space_sector new_sec)
{
	//Local position + (sector displacement on each axis) * (size of a sector)
	return pos + (vec3){sec.x - new_sec.x, sec.y - new_sec.y, sec.z - new_sec.z} * SPACE_SECTOR_SIZE; //lol secx
}