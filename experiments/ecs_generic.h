#ifndef ECS_GENERIC_H
#define ECS_GENERIC_H

#include "datastructures/ecs.h"
#include "components/components.h"
#include <inttypes.h>

//Adds some type safety and convenience.
//Define ECS_ACTIVE_ECS and appropriate entity cids prior to including this file.
#define entity_add_component(eid, component) _Generic((component),          \
	uint32_t:     ecs_entity_add_component(ECS_ACTIVE_ECS, eid, component), \
	default:      ecs_entity_add_copy_component(ECS_ACTIVE_ECS, eid, _Generic((component), \
		Controllable: ENTITY_CONTROLLABLE_CID, &component, \
		Physical:     ENTITY_PHYSICAL_CID,     &component, \
		Scriptable:   ENTITY_SCRIPTABLE_CID,   &component, \
		Universal:    ENTITY_UNIVERSAL_CID,    &component  \
		)))

#define entity_get(eid, component) ecs_entity_get_component(ECS_ACTIVE_ECS, eid, ctype)

//Convenience macros for accessing different component types
#define entity_controllable(eid) (Controllable *)entity_get(eid, ENTITY_CONTROLLABLE_CID)
#define entity_physical(eid) (Physical *)entity_get(eid, ENTITY_PHYSICAL_CID)
#define entity_scriptable(eid) (Scriptable *)entity_get(eid, ENTITY_SCRIPTABLE_CID)

#endif