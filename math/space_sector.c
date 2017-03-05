#include <math.h>
#include "space_sector.h"
#include "glla.h"
#include "stdio.h"

const float SPACE_SECTOR_SIZE = 16384;

void space_sector_canonicalize(vec3 *pos, space_sector *sec)
{
	vec3 pos_copy = *pos;
	space_sector sec_copy = *sec;
	sec->x += (long long int)(pos->x / SPACE_SECTOR_SIZE);
	sec->y += (long long int)(pos->y / SPACE_SECTOR_SIZE);
	sec->z += (long long int)(pos->z / SPACE_SECTOR_SIZE);

	pos->x = fmod(pos->x, SPACE_SECTOR_SIZE);
	pos->y = fmod(pos->y, SPACE_SECTOR_SIZE);
	pos->z = fmod(pos->z, SPACE_SECTOR_SIZE);

	if (pos->x != pos_copy.x || pos->y != pos_copy.y || pos->z != pos_copy.z || sec_copy.x != sec->x || sec_copy.y != sec->y || sec_copy.z != sec->z)
		printf("Position renormalized from [%lli, %lli, %lli](%f, %f, %f) to [%lli, %lli, %lli](%f, %f, %f)\n",
			sec_copy.x, sec_copy.y, sec_copy.z,
			pos_copy.x, pos_copy.y, pos_copy.z,
			sec->x, sec->y, sec->z,
			pos->x, pos->y, pos->z);
}

//Also copied to stars.vs, so update there too if changed.
vec3 space_sector_position_relative_to_sector(vec3 pos, space_sector sec, space_sector new_sec)
{
	//Local position + (sector displacement on each axis) * (size of a sector)
	return pos + (vec3){sec.x - new_sec.x, sec.y - new_sec.y, sec.z - new_sec.z} * 2 * SPACE_SECTOR_SIZE; //lol secx
}