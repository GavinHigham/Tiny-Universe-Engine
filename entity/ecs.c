/*
ECS

Rough notes on new entity component system implementation

Components are just data (fixed size)
Entities are just sets of components.
Systems operate over sets of components, leveraging a separate component-to-entity mapping to find other needed components attached to the same entity.

Mapping from component to entity:
Entity * ecs_c_to_entity(ECS *E, int component_id, int component_index)
Just need a second memory pool of pointers or indexes of containing Entity, make sure component pool operations are also done on the pointer
(Should this be optional behavior? Could be convenient to allow embedded pointers for components that are expected to only be updated in conjunction with other components.)

ECS will take an entity id and return a pointer to a list of component indexes (or pointers)
ECS helper can return a struct that's defined to contain pointers to all compile-time components, for convenient C access. Could look like this:
#include "ecs/ecs.h"
#include "ecs/ecs_c.h"
...
Physical *p = ecs_entity_c(E, eid).physical;
RuntimeComponent *r = ecs_entity(E, eid, cid); //Only while porting Lua component to C component, or accessing entity from Lua.


//Add a new component type to the ecs. Provide the size of the component, ecs will allocate storage for it and create a lookup table mapping back to the entity.
//
ecs_add_component
ecs_add_system
*/

typedef struct ecs_context {
	//Mapping from entity ID % entity limit to entity component list (entity component list should have entity ID to make sure it's not an old entity ID)
	int *eid_to_entity;
	//Component list pool (overprovision at start, reallocate if we add too many dynamic systems)
	struct mempool entities;
	//Mapping from component ID to component pool
	int *cid_to_component_pool;
	int num_component_types;
	//Component pool pool
	struct mempool components;
} ecs_ctx;

ecs_ctx * ecs_new(int num_entities, int num_component_types)
{
	ecs_ctx e = {
		.eid_to_entity = malloc(sizeof(int) * num_entities),
		.entities = mempool_new(num_entities, sizeof(void *)*num_component_types),
		.cid_to_component_pool = malloc(sizeof(int) * num_component_types),
		.num_component_types = num_component_types,
		.components = mempool_new(num_component_types, sizeof(void *))
	}
}

//Returns cid
int ecs_add_component(ecs_ctx *E, size_t num, size_t size)
{
	struct mempool *C = &E->components;
	if (C->num >= C->max)
		//Reallocate components
		//Reallocate cid_to_component_pool
		//Reallocate entities
		//Correct entities pointers

	int cid = C->num;
	E->cid_to_component_pool[cid] = mempool_add(C, malloc(num * size));
}