#ifndef COMPONENTS_GENERIC_H
#define COMPONENTS_GENERIC_H

#include "datastructures/ecs.h"
#include "components/components.h"
#include <inttypes.h>

//Define ECS_ACTIVE_ECS and ECS_ACTIVE_CTYPES before including this file.

//Used to look up ctype by component name
struct entity_ctypes {
	uint32_t camera;
	uint32_t controllable;
	uint32_t customdrawable;
	uint32_t label;
	uint32_t physical;
	uint32_t scriptable;
	uint32_t universal;
};

#define entity_new() ecs_entity_add(ECS_ACTIVE_ECS)

//Adds some type safety and convenience.
//Define ECS_ACTIVE_ECS and appropriate entity cids prior to including this file.
#define entity_add(eid, component) ecs_entity_add_copy_component(ECS_ACTIVE_ECS, eid, _Generic((component), \
		Camera:           ECS_ACTIVE_CTYPES->camera,         \
		ControllableTemp: ECS_ACTIVE_CTYPES->controllable,   \
		CustomDrawable:   ECS_ACTIVE_CTYPES->customdrawable, \
		Label:            ECS_ACTIVE_CTYPES->label,          \
		PhysicalTemp:     ECS_ACTIVE_CTYPES->physical,       \
		ScriptableTemp:   ECS_ACTIVE_CTYPES->scriptable,     \
		Universal:        ECS_ACTIVE_CTYPES->universal       \
		), &(component))


#define entity_get(eid, ctype) ecs_entity_get_component(ECS_ACTIVE_ECS, eid, ctype)

//Convenience macros for accessing and casting to different component types
#define entity_camera(eid)         (Camera *)         entity_get(eid, ECS_ACTIVE_CTYPES->camera_cid)
#define entity_controllable(eid)   (Controllable *)   entity_get(eid, ECS_ACTIVE_CTYPES->controllable_cid)
#define entity_customdrawable(eid) (CustomDrawable *) entity_get(eid, ECS_ACTIVE_CTYPES->customdrawable_cid)
#define entity_label(eid)          (Label *)          entity_get(eid, ECS_ACTIVE_CTYPES->label_cid)
#define entity_physical(eid)       (Physical *)       entity_get(eid, ECS_ACTIVE_CTYPES->physical_cid)
#define entity_physicaltemp(eid)   (PhysicalTemp *)   entity_get(eid, ECS_ACTIVE_CTYPES->physical_cid)
#define entity_scriptable(eid)     (Scriptable *)     entity_get(eid, ECS_ACTIVE_CTYPES->scriptable_cid)

#endif