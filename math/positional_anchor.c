#include <glla.h>
#include <math.h>
#include "positional_anchor.h"

positional_anchor positional_anchor_canonicalize(positional_anchor p)
{
	p.sector.x += p.pos.x / POSITIONAL_ANCHOR_SECTOR_SIZE;
	p.sector.y += p.pos.y / POSITIONAL_ANCHOR_SECTOR_SIZE;
	p.sector.z += p.pos.z / POSITIONAL_ANCHOR_SECTOR_SIZE;

	p.pos = (vec3){
		fmod(p.pos.x, POSITIONAL_ANCHOR_SECTOR_SIZE),
		fmod(p.pos.y, POSITIONAL_ANCHOR_SECTOR_SIZE),
		fmod(p.pos.z, POSITIONAL_ANCHOR_SECTOR_SIZE)
	};

	return p;
}

vec3 positional_anchor_local_position_relative_to_sector(positional_anchor p, positional_anchor_sector s)
{
	//Local position + (sector displacement on each axis) * (size of a sector)
	return p.pos + (vec3){p.sector.x - s.x, p.sector.y - s.y, p.sector.z - s.z} * POSITIONAL_ANCHOR_SECTOR_SIZE;	
}