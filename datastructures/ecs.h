#ifndef ECS_H
#define ECS_H
#include "mempool.h"
#include "hmempool.h"
#include <inttypes.h>

typedef void (*ecs_c_constructor_fn)(uint32_t eid, void *component, void *userdata);
typedef void (*ecs_c_destructor_fn)(uint32_t eid, void *component, void *userdata);
struct ecs_component_init_params;
typedef struct ecs_context {
	struct hmempool entities;
	//Stack of free component slots
	struct mempool free_component_slots;
	//Component pool array
	struct hmempool *components;
	//Mapping of anything to ctype, not maintained by ecs. Convenience for generic wrapper.
	void *user_ctypes;
	ecs_c_constructor_fn *constructors;
	ecs_c_destructor_fn *destructors;
	void **userdatas;
	struct ecs_component_init_params **init_params;
} ecs_ctx;

struct ecs_component_init_params {
	size_t num, size;
	ecs_c_constructor_fn construct;
	ecs_c_destructor_fn destruct;
	void *userdata;
	uint32_t *ctype;
	ecs_ctx *E;
	void *ctx;
	struct ecs_component_init_params *ip;
	void *(*init)(struct ecs_component_init_params *);
	void (*deinit)(struct ecs_component_init_params *);
};

//Current maximum number of entities, will reallocate if number goes above this.
size_t ecs_entities_max(ecs_ctx *E) __attribute__ ((pure));
//Current number of entities.
size_t ecs_entities_num(ecs_ctx *E) __attribute__ ((pure));
//Maximum number of registered components, will reallocate if number goes above this.
size_t ecs_components_max(ecs_ctx *E) __attribute__ ((pure));
//Current number of registered components.
size_t ecs_components_num(ecs_ctx *E) __attribute__ ((pure));
//Create a new ecs_ctx
//num_entities: Initial number of entities ecs can hold.
//num_component_types: Initial number of types of components per entity.
ecs_ctx ecs_new(int num_entities, int num_component_types);
//Free data from ecs_ctx. Using the ecs_ctx after calling ecs_free on it is undefined behavior.
void ecs_free(ecs_ctx *E);
//Get the list of components for a given eid.
//!!! IMPORTANT !!! The returned pointer may become invalid under many circumstances:
// 1. If any entities are added or removed from the ecs.
// 2. If the ecs is reallocated or resized.
// 3. If a new component type is registered.
//Make sure to get a fresh list if other operations have been performed on the ecs.
//It will not be invalidated if components are added or removed from any entity (including this one).
//List will be the length returned by ecs_components_max, indexed by ctype.
//A handle with value of 0 indicates that the entity does not have a component of that type.
uint32_t * ecs_component_list(ecs_ctx *E, uint32_t eid) __attribute__ ((pure));;
//Reallocate the ecs_ctx so it can hold num_entities and num_component_types.
void ecs_realloc(ecs_ctx *E, int num_entities, int num_component_types);
//Register a new type of component.
//num: Initialize storage for this number of components of that type.
//size: Each component of this type is this many bytes in size.
//Returns the ctype, used to index an entity's component list.
uint32_t ecs_component_register(ecs_ctx *E, size_t num, size_t size);
//Register and initialize a new type of component, passing an init params struct with these members:
//!!! IMPORTANT !!! p should remain valid for the lifetime of the ECS. If p is freed, undefined behavior will occur.
//num: Initialize storage for this number of components of that type.
//size: Each component of this type is this many bytes in size.
//Any of the following can be NULL.
//construct: Constructor that will be called on new component instances after they are allocated.
//destruct: Destructor that will be called on component right before it is destroyed.
//userdata: Pointer passed to constructor and destructor, usually used to interact with resources not managed by ECS.
//ctype: If not NULL, *ctype will be set to the ctype that is returned from this call.
//E: Pointer to the ECS context, will be set before init is called.
//ctx: Will store the return value of init after it is called.
//ip: Will be set to p before init is called.
//init: Initialization function for component runtime, will be called and passed p after component registration.
//deinit: Deinitialization function for component runtime, will be called and passed p when component is unregistered or ECS is freed.
uint32_t ecs_component_init(ecs_ctx *E, struct ecs_component_init_params *p);
//Sets a constructor and destructor function for that component type.
//construct: Constructor that will be called on new component instances after they are allocated.
//destruct: Destructor that will be called on component right before it is destroyed.
//userdata: Passed to the constructor and destructor functions, usually used to free resources not managed by the ECS.
void ecs_component_set_construct_destruct(ecs_ctx *E, uint32_t ctype,
	ecs_c_constructor_fn construct, ecs_c_destructor_fn destruct, void *userdata);
//Unregister a type of component. All entities will have components of that type removed from them.
//The ctype may be recycled for future registered component types.
void ecs_component_unregister(ecs_ctx *E, int ctype);
//Add a new empty entity to the ecs.
uint32_t ecs_entity_add(ecs_ctx *E);
//Remove the entity with handle "eid" from the ecs.
//The entity will be immediately overwritten.
void ecs_entity_remove(ecs_ctx *E, uint32_t eid);
//Add a new component of type ctype to the entity with handle "eid".
//Returns a pointer to the new component, for initialization.
void * ecs_entity_add_component(ecs_ctx *E, int eid, int ctype);
//Add a new component of type ctype to the entity with handle "eid".
//Returns a pointer to the new component, for initialization.
//Calls the registered constructor for ctype, if any.
void * ecs_entity_add_construct_component(ecs_ctx *E, int eid, int ctype);
//Add a new component of type ctype to the entity with handle "eid".
//Copies component into ECS.
//Returns a pointer to the new component, for initialization.
void * ecs_entity_add_copy_component(ecs_ctx *E, int eid, int ctype, void *c);
//Add a new component of type ctype to the entity with handle "eid".
//Copies component into ECS.
//Calls the registered constructor for ctype, if any.
//Returns a pointer to the new component, for initialization.
void * ecs_entity_add_construct_copy_component(ecs_ctx *E, int eid, int ctype, void *c);
//Remove the component of type ctype from the entity with handle "eid".
//The component will immediately be overwritten.
void ecs_entity_remove_component(ecs_ctx *E, int eid, int ctype);
//Remove all components from the entity with handle "eid".
//All components will immediately be overwritten.
void ecs_entity_remove_components(ecs_ctx *E, int eid);
//Get the component of type ctype from the entity with handle "eid".
//Returns a pointer to the new component, or NULL if the entity does not have a component of that type.
void * ecs_entity_get_component(ecs_ctx *E, int eid, int ctype) __attribute__ ((pure));

#endif