#include "entity/entity_components.h"
#include "space/star_box.h"
#include "space/solar_system.h"

//TODO(Gavin): These should all be encapsulated by an entity that represents the solar system
extern solar_system ssystem;
extern qvec3 solar_system_origin;
extern bool gen_solar_systems;
extern int64_t solar_system_star;

scriptable_callback(galaxy_box_script)
{
	
}