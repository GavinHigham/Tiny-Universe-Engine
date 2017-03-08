#include <stdio.h>
#include <math.h>
#include <string.h>
#include "space_sector.h"
#include "glla.h"

const float SPACE_SECTOR_SIZE = 8192;
static const float SPACE_SECTOR_RADIUS = SPACE_SECTOR_SIZE / 2;

void space_sector_canonicalize(vec3 *pos, space_sector *sec)
{
	//vec3 pos_old = *pos;
	//space_sector sec_old = *sec;
	space_sector new = *sec;
	new.x += truncf((pos->x + copysignf(SPACE_SECTOR_RADIUS, pos->x)) / SPACE_SECTOR_SIZE);
	new.y += truncf((pos->y + copysignf(SPACE_SECTOR_RADIUS, pos->y)) / SPACE_SECTOR_SIZE);
	new.z += truncf((pos->z + copysignf(SPACE_SECTOR_RADIUS, pos->z)) / SPACE_SECTOR_SIZE);

	// pos->x = fmod(pos->x, SPACE_SECTOR_SIZE);
	// pos->y = fmod(pos->y, SPACE_SECTOR_SIZE);
	// pos->z = fmod(pos->z, SPACE_SECTOR_SIZE);
	*pos = space_sector_position_relative_to_sector(*pos, *sec, new);
	*sec = new;

	//Print test code
	// if (pos->x != pos_old.x || pos->y != pos_old.y || pos->z != pos_old.z || sec_old.x != sec->x || sec_old.y != sec->y || sec_old.z != sec->z) {
	// 	printf("Position renormalized from ");
	// 	space_sector_print(sec_old);
	// 	vec3_print(pos_old);
	// 	printf(" to ");
	// 	space_sector_print(*sec);
	// 	vec3_print(*pos);
	// 	puts("");
	// }

	//Branch-based attempt
	// sec->x += truncf(pos->x / SPACE_SECTOR_SIZE);
	// sec->y += truncf(pos->y / SPACE_SECTOR_SIZE);
	// sec->z += truncf(pos->z / SPACE_SECTOR_SIZE);

	// pos->x = fmod(pos->x, SPACE_SECTOR_SIZE);
	// pos->y = fmod(pos->y, SPACE_SECTOR_SIZE);
	// pos->z = fmod(pos->z, SPACE_SECTOR_SIZE);

	// if (pos->x < 0) { pos->x += SPACE_SECTOR_SIZE; sec->x--; };
	// if (pos->y < 0) { pos->y += SPACE_SECTOR_SIZE; sec->y--; };
	// if (pos->z < 0) { pos->z += SPACE_SECTOR_SIZE; sec->z--; };
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
