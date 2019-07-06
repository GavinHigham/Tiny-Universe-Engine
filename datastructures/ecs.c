#include "ecs.h"
#include "mempool.h"
#include "hmempool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void ecs_call_destructor(ecs_ctx *E, uint32_t eid, uint32_t ctype, void *component)
{
	if (E->destructors[ctype])
		E->destructors[ctype](eid, component, E->userdatas[ctype]);
}

size_t ecs_entities_max(ecs_ctx *E)
{
	return E->entities.pool.max;
}

size_t ecs_entities_num(ecs_ctx *E)
{
	return E->entities.pool.num;
}

size_t ecs_components_max(ecs_ctx *E)
{
	return E->free_component_slots.max;
}

size_t ecs_components_num(ecs_ctx *E)
{
	return E->free_component_slots.max - E->free_component_slots.num;
}

uint32_t * ecs_component_list(ecs_ctx *E, uint32_t h)
{
	return hmempool_get(&E->entities, h);
}

ecs_ctx ecs_new(int num_entities, int num_component_types)
{
	ecs_ctx tmp = {
		.free_component_slots = mempool_new(num_component_types, sizeof(uint32_t)),
		.entities = hmempool_new(num_entities, sizeof(uint32_t) * num_component_types),
		.components = calloc(num_component_types, sizeof(struct hmempool)),
		.constructors = calloc(num_component_types, sizeof(ecs_c_constructor_fn)),
		.destructors = calloc(num_component_types, sizeof(ecs_c_destructor_fn)),
		.userdatas = calloc(num_component_types, sizeof(void *)),
		.init_params = calloc(num_component_types, sizeof(struct ecs_component_init_params *)),
	};
	for (uint32_t i = 0; i < num_component_types; i++)
		mempool_add(&tmp.free_component_slots, &i);
	return tmp;
}

void ecs_free(ecs_ctx *E)
{
	uint32_t num = ecs_entities_max(E);
	for (int i = 0; i < num; i++) {
		//Iterate over entity indices, retrieve live entity handles.
		uint32_t eid = E->entities.itoh[i];
		//PERF(Gavin): This is kind of inefficient (O(n) with the max number of entities, and lots of indirection)
		if (eid)
			ecs_entity_remove_components(E, eid);
	}

	//Unregister all components, call component deinit if present.
	for (int i = 0; i < ecs_components_max(E); i++)
		if (E->init_params[i])
			E->init_params[i]->deinit(E->init_params[i]);

	//Shouldn't need to unregister components because all component destructors have already been called by this point.

	for (int i = 0; i < ecs_components_max(E); i++)
		hmempool_delete(&E->components[i]);
	free(E->components);
	free(E->constructors);
	free(E->destructors);
	free(E->userdatas);
	free(E->init_params);
	hmempool_delete(&E->entities);
	mempool_delete(&E->free_component_slots);
}

void ecs_realloc(ecs_ctx *E, int num_entities, int num_component_types)
{
	size_t cmax = ecs_components_max(E);
	if (num_component_types > cmax || num_entities > ecs_entities_max(E))
		hmempool_resize_stretch(&E->entities, num_entities, sizeof(uint32_t) * num_component_types);
	if (num_component_types > cmax) {

		mempool_resize(&E->free_component_slots, num_component_types);
		for (uint32_t i = cmax; i < num_component_types; i++)
			mempool_add(&E->free_component_slots, &i);

		struct hmempool *new_components = realloc(E->components, num_component_types * sizeof(struct hmempool));
		if (new_components)
			E->components = new_components;
		else
			printf("Whoops, running out of memory.\n");

		ecs_c_constructor_fn *new_constructors = realloc(E->constructors, num_component_types * sizeof(ecs_c_constructor_fn));
		if (new_constructors) {
			E->constructors = new_constructors;
			for (int i = cmax; i < num_component_types; i++)
				E->constructors[i] = NULL;
		} else {
			printf("Whoops, running out of memory.\n");
		}

		ecs_c_destructor_fn *new_destructors = realloc(E->destructors, num_component_types * sizeof(ecs_c_destructor_fn));
		if (new_destructors) {
			E->destructors = new_destructors;
			for (int i = cmax; i < num_component_types; i++)
				E->destructors[i] = NULL;
		}
		else {
			printf("Whoops, running out of memory.\n");
		}

		void **new_userdatas = realloc(E->userdatas, num_component_types * sizeof(void *));
		if (new_userdatas) {
			E->userdatas = new_userdatas;
			for (int i = cmax; i < num_component_types; i++)
				E->userdatas[i] = NULL;
		}
		else {
			printf("Whoops, running out of memory.\n");
		}

		struct ecs_component_init_params **new_init_params = realloc(E->init_params, num_component_types * sizeof(struct ecs_component_init_params *));
		if (new_init_params) {
			E->init_params = new_init_params;
			for (int i = cmax; i < num_component_types; i++)
				E->init_params[i] = NULL;
		}
		else {
			printf("Whoops, running out of memory.\n");
		}
	}
}

//Returns ctype
uint32_t ecs_component_register(ecs_ctx *E, size_t num, size_t size)
{
	if (E->free_component_slots.num == 0)
		ecs_realloc(E, ecs_entities_max(E), ecs_components_max(E) * 2);

	uint32_t ctype;
	mempool_pop(&E->free_component_slots, &ctype);

	E->components[ctype] = hmempool_new(num, size);
	return ctype;
}

//Register and initialize a new type of component, passing an init params struct with these members:
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
uint32_t ecs_component_init(ecs_ctx *E, struct ecs_component_init_params *p)
{
	uint32_t ctype = ecs_component_register(E, p->num, p->size);
	ecs_component_set_construct_destruct(E, ctype, p->construct, p->destruct, p->userdata);
	if (p->ctype)
		*p->ctype = ctype;
	p->E = E;
	p->ip = p;
	p->ctx = p->init(p);
	E->init_params[ctype] = p;
	return ctype;
}

void ecs_component_set_construct_destruct(ecs_ctx *E, uint32_t ctype,
	ecs_c_constructor_fn construct, ecs_c_destructor_fn destruct, void *userdata)
{
	E->constructors[ctype] = construct;
	E->destructors[ctype] = destruct;
	E->userdatas[ctype] = userdata;
}

void ecs_component_unregister(ecs_ctx *E, int ctype)
{
	uint32_t num = ecs_entities_max(E);
	//Remove that component from any entity that has it.
	for (int i = 0; i < num; i++) {
		//Iterate over entity indices, retrieve live entity handles.
		uint32_t eid = E->entities.itoh[i];
		if (eid) {
			uint32_t *cl = ecs_component_list(E, eid);
			if (cl[ctype]) {
				ecs_call_destructor(E, eid, ctype, hmempool_get(&E->components[ctype], cl[ctype]));
				cl[ctype] = 0;
			}
		}
	}
	if (E->init_params[ctype])
		E->init_params[ctype]->deinit(E->init_params[ctype]);
	E->init_params[ctype] = NULL;

	hmempool_delete(&E->components[ctype]);
	mempool_add(&E->free_component_slots, &ctype);
}

uint32_t ecs_entity_add(ecs_ctx *E)
{
	if (E->entities.pool.num >= E->entities.pool.max)
		ecs_realloc(E, ecs_entities_max(E) * 2, ecs_components_max(E));

	//At this point, we know there must be at least one free slot.

	//For now, eids are just hmempool handles.
	//Later, I can bitwise-or in more information to the handle, if needed.
	uint32_t h = hmempool_add_raw(&E->entities);
	memset(hmempool_get(&E->entities, h), 0, E->entities.pool.size);
	return h;
}

void ecs_entity_remove(ecs_ctx *E, uint32_t eid)
{
	uint32_t *cl = ecs_component_list(E, eid);
	//Remove all components
	for (int i = 0; i < ecs_components_max(E); i++) {
		//If it's a valid component handle, remove and clear it.
		uint32_t ch = cl[i];
		if (ch) {
			ecs_call_destructor(E, eid, i, hmempool_get(&E->components[i], ch));
			hmempool_remove(&E->components[i], ch);
		}
	}

	hmempool_remove(&E->entities, eid);
}

void * ecs_entity_add_component(ecs_ctx *E, int eid, int ctype)
{
	//Get the component list, resize if needed.
	struct hmempool *cl = &E->components[ctype];
	if (cl->pool.num >= cl->pool.max)
		hmempool_resize(&E->components[ctype], cl->pool.max * 2);

	//Get the new component handle
	uint32_t ch = hmempool_add_raw(cl);
	//Attach component to entity.
	assert(!ecs_component_list(E, eid)[ctype]);
	ecs_component_list(E, eid)[ctype] = ch;
	return hmempool_get(&E->components[ctype], ch);
}

void * ecs_entity_add_construct_component(ecs_ctx *E, int eid, int ctype)
{
	void *c = ecs_entity_add_component(E, eid, ctype);
	if (E->constructors[ctype])
		E->constructors[ctype](eid, c, E->userdatas[ctype]);
	return c;
}

void * ecs_entity_add_copy_component(ecs_ctx *E, int eid, int ctype, void *c)
{
	//Get the component list, resize if needed.
	struct hmempool *cl = &E->components[ctype];
	if (cl->pool.num >= cl->pool.max)
		hmempool_resize(&E->components[ctype], cl->pool.max * 2);

	//Get the new component handle
	uint32_t ch = hmempool_add(cl, c);
	//Attach component to entity.
	assert(!ecs_component_list(E, eid)[ctype]);
	ecs_component_list(E, eid)[ctype] = ch;
	return hmempool_get(&E->components[ctype], ch);
}

void * ecs_entity_add_construct_copy_component(ecs_ctx *E, int eid, int ctype, void *c)
{
	void *tmp = ecs_entity_add_copy_component(E, eid, ctype, c);
	if (E->constructors[ctype])
		E->constructors[ctype](eid, tmp, E->userdatas[ctype]);
	return tmp;
}

void ecs_entity_remove_component(ecs_ctx *E, int eid, int ctype)
{
	uint32_t *c = ecs_component_list(E, eid);
	uint32_t ch = c[ctype];
	if (ch) {
		struct hmempool *chm = &E->components[ctype];
		ecs_call_destructor(E, eid, ctype, hmempool_get(chm, ch));
		hmempool_remove(chm, ch);
		c[ctype] = 0;
	}
}

void ecs_entity_remove_components(ecs_ctx *E, int eid)
{
	//Simple solution for now, can optimize a bit later.
	for (int i = 0; i < ecs_components_max(E); i++)
		ecs_entity_remove_component(E, eid, i);
}

void * ecs_entity_get_component(ecs_ctx *E, int eid, int ctype)
{
	uint32_t c = ecs_component_list(E, eid)[ctype];
	if (c)
		return hmempool_get(&E->components[ctype], c);
	return NULL;
}
