#ifndef COMPONENTS_GENERIC_H
#define COMPONENTS_GENERIC_H

#include "datastructures/ecs.h"
#include "components/components.h"
#include <inttypes.h>

//Define ECS_ACTIVE_ECS and ECS_ACTIVE_CTYPES before including this file.

//Used to look up ctype by component name. Safe to re-order, keep it alphabetical.
struct entity_ctypes {
	uint32_t camera;
	uint32_t controllable;
	uint32_t customdrawable;
	uint32_t framebuffer;
	uint32_t label;
	uint32_t physical;
	uint32_t scriptable;
	uint32_t target;
	uint32_t universal;
};

#define entity_new() ecs_entity_add(ECS_ACTIVE_ECS)

//Adds some type safety and convenience.
//Define ECS_ACTIVE_ECS and appropriate entity cids prior to including this file.
//"component..." used instead of "component" to work around a limitation where macros get confused by commas in inline compound literals.
#define entity_add(eid, component...) _Generic((component), \
		Camera:           ecs_entity_add_copy_component(ECS_ACTIVE_ECS, eid, ECS_ACTIVE_CTYPES->camera,                   &(component)),\
		ControllableTemp: ecs_entity_add_copy_component(ECS_ACTIVE_ECS, eid, ECS_ACTIVE_CTYPES->controllable,             &(component)),\
		CustomDrawable:   ecs_entity_add_copy_construct_component(ECS_ACTIVE_ECS, eid, ECS_ACTIVE_CTYPES->customdrawable, &(component)),\
		Framebuffer:      ecs_entity_add_copy_construct_component(ECS_ACTIVE_ECS, eid, ECS_ACTIVE_CTYPES->framebuffer,    &(component)),\
		Label:            ecs_entity_add_copy_construct_component(ECS_ACTIVE_ECS, eid, ECS_ACTIVE_CTYPES->label,          &(component)),\
		PhysicalTemp:     ecs_entity_add_copy_component(ECS_ACTIVE_ECS, eid, ECS_ACTIVE_CTYPES->physical,                 &(component)),\
		ScriptableTemp:   ecs_entity_add_copy_component(ECS_ACTIVE_ECS, eid, ECS_ACTIVE_CTYPES->scriptable,               &(component)),\
		Target:           ecs_entity_add_copy_component(ECS_ACTIVE_ECS, eid, ECS_ACTIVE_CTYPES->target,                   &(component)),\
		Universal:        ecs_entity_add_copy_component(ECS_ACTIVE_ECS, eid, ECS_ACTIVE_CTYPES->universal,                &(component))) //Last one has paren instead of comma

#define entity_get(eid, ctype) ecs_entity_get_component(ECS_ACTIVE_ECS, eid, ctype)

//Convenience macros for accessing and casting to different component types
#define entity_camera(eid)         ((Camera *)         entity_get(eid, ECS_ACTIVE_CTYPES->camera))
#define entity_controllable(eid)   ((Controllable *)   entity_get(eid, ECS_ACTIVE_CTYPES->controllable))
#define entity_customdrawable(eid) ((CustomDrawable *) entity_get(eid, ECS_ACTIVE_CTYPES->customdrawable))
#define entity_framebuffer(eid)    ((Framebuffer *)    entity_get(eid, ECS_ACTIVE_CTYPES->framebuffer))
#define entity_label(eid)          ((Label *)          entity_get(eid, ECS_ACTIVE_CTYPES->label))
#define entity_physical(eid)       ((Physical *)       entity_get(eid, ECS_ACTIVE_CTYPES->physical))
#define entity_physicaltemp(eid)   ((PhysicalTemp *)   entity_get(eid, ECS_ACTIVE_CTYPES->physical))
#define entity_scriptable(eid)     ((Scriptable *)     entity_get(eid, ECS_ACTIVE_CTYPES->scriptable))
#define entity_target(eid)         ((Target *)         entity_get(eid, ECS_ACTIVE_CTYPES->target))

#define entity_remove_customdrawable(eid) ecs_entity_destruct_remove_component(ECS_ACTIVE_ECS, eid, ECS_ACTIVE_CTYPES->customdrawable)

#endif