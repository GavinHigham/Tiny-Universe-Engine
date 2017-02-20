#include <glla.h>

float POSITIONAL_ANCHOR_SECTOR_SIZE = 8192; //2^13 meters.
typedef struct {
	long long int x, y, z;
} positional_anchor_sector;

typedef struct {
	positional_anchor_sector sector;
	vec3 pos;
} positional_anchor;

positional_anchor positional_anchor_canonicalize(positional_anchor p);
positional_anchor_sector positional_anchor_get_sector(positional_anchor p);
vec3 positional_anchor_local_position_relative_to_sector(positional_anchor p, positional_anchor_sector s);