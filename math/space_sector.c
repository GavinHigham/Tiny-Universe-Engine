#include <math.h>
#include "space_sector.h"
#include "glla.h"

const float SPACE_SECTOR_SIZE = 8192;

void space_sector_canonicalize(vec3 *pos, space_sector *sec)
{
	sec->x += pos->x / SPACE_SECTOR_SIZE;
	sec->y += pos->y / SPACE_SECTOR_SIZE;
	sec->z += pos->z / SPACE_SECTOR_SIZE;

	*pos = (vec3){
		fmod(pos->x, SPACE_SECTOR_SIZE),
		fmod(pos->y, SPACE_SECTOR_SIZE),
		fmod(pos->z, SPACE_SECTOR_SIZE)
	};
}

//Also copied to stars.vs, so update there too if changed.
vec3 space_sector_position_relative_to_sector(vec3 pos, space_sector sec, space_sector new_sec)
{
	//Local position + (sector displacement on each axis) * (size of a sector)
	return pos + (vec3){sec.x - new_sec.x, sec.y - new_sec.y, sec.z - new_sec.z} * SPACE_SECTOR_SIZE; //lol secx
}