#include <glla.h>

//2^13 meters. Distance from the center of the sector to the edge along any axis.
const float SPACE_SECTOR_SIZE = 8192;
typedef struct {
	long long int x, y, z;
} space_sector;

//If pos is outside the bounds of sec, sets sec to the correct sector enclosing pos, and makes pos relative to that origin.
void space_sector_canonicalize(vec3 *pos, space_sector *sec);
//If pos is expressed relative to sec, returns pos relative to new_sec.
vec3 space_sector_position_relative_to_sector(vec3 pos, space_sector sec, space_sector new_sec);