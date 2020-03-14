#include "ecs.h"
#include "mempool.h"
#include "hmempool.h"
#include "math/utility.h"
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
	return E->free_eids.max;
}

size_t ecs_entities_num(ecs_ctx *E)
{
	return E->free_eids.max - E->free_eids.num;
}

size_t ecs_components_max(ecs_ctx *E)
{
	return E->free_component_slots.max;
}

size_t ecs_components_num(ecs_ctx *E)
{
	return E->free_component_slots.max - E->free_component_slots.num;
}

bool ecs_eid_used(ecs_ctx *E, uint32_t eid)
{
	//Could do bounds / validity check on eid, possibly.
	return E->eid_used[eid];
}

ecs_ctx ecs_new(int num_entities, int num_component_types)
{
	ecs_ctx tmp = {
		.free_component_slots = mempool_new(num_component_types, sizeof(uint32_t)),
		.free_eids = mempool_new(num_entities, sizeof(uint32_t)),
		.eid_used = calloc(num_entities + 1, sizeof(bool)),
		.components = calloc(num_component_types, sizeof(struct hmempool)),
		.constructors = calloc(num_component_types, sizeof(ecs_c_constructor_fn *)),
		.destructors = calloc(num_component_types, sizeof(ecs_c_destructor_fn *)),
		.userdatas = calloc(num_component_types, sizeof(void *)),
		.init_params = calloc(num_component_types, sizeof(struct ecs_component_init_params *)),
	};
	for (uint32_t i = 0; i < num_component_types; i++)
		mempool_add(&tmp.free_component_slots, &i);
	mempool_fill_uint32_t_descending(&tmp.free_eids, 1, num_entities);
	return tmp;
}

void ecs_free(ecs_ctx *E)
{
	uint32_t num = ecs_entities_max(E);
	for (int i = 1; i < num + 1; i++) {
		//PERF(Gavin): This is kind of inefficient (O(n) with max number of entities, lots of indirection, jumps around memory a lot)
		if (ecs_eid_used(E, i))
			ecs_entity_remove_components(E, i);
	}

	//Unregister all components, call component deinit if present.
	for (int i = 0; i < ecs_components_max(E); i++)
		if (E->init_params[i] && E->init_params[i]->deinit)
			E->init_params[i]->deinit(E->init_params[i]);

	//Shouldn't need to unregister components because all component destructors have already been called by this point.

	for (int i = 0; i < ecs_components_max(E); i++)
		hmempool_delete(&E->components[i]);
	free(E->eid_used);
	free(E->components);
	free(E->constructors);
	free(E->destructors);
	free(E->userdatas);
	free(E->init_params);
	mempool_delete(&E->free_eids);
	mempool_delete(&E->free_component_slots);
}

void ecs_realloc(ecs_ctx *E, int num_entities, int num_component_types)
{
	size_t emax = ecs_entities_max(E);
	size_t cmax = ecs_components_max(E);
	if (num_entities > emax) {
		for (int i = 0; i < cmax; i++)
			hmempool_handles_resize(&E->components[i], num_entities);
		mempool_resize(&E->free_eids, num_entities);
		mempool_fill_uint32_t_descending(&E->free_eids, emax+1, num_entities);
		bool *new_eid_used = crealloc(E->eid_used, num_entities + 1, emax);
		if (new_eid_used)
			E->eid_used = new_eid_used;
		else
			printf("Whoops, running out of memory.\n");
	}
	if (num_component_types > cmax) {
		mempool_resize(&E->free_component_slots, num_component_types);
		for (uint32_t i = cmax; i < num_component_types; i++)
			mempool_add(&E->free_component_slots, &i);

		struct hmempool *new_components = realloc(E->components, num_component_types * sizeof(struct hmempool));
		if (new_components)
			E->components = new_components;
		else
			printf("Whoops, running out of memory.\n");

		ecs_c_constructor_fn **new_constructors = crealloc(E->constructors, num_component_types * sizeof(ecs_c_constructor_fn *), cmax * sizeof(ecs_c_constructor_fn *));
		if (new_constructors)
			E->constructors = new_constructors;
		else
			printf("Whoops, running out of memory.\n");

		ecs_c_destructor_fn **new_destructors = crealloc(E->destructors, num_component_types * sizeof(ecs_c_destructor_fn *), cmax * sizeof(ecs_c_destructor_fn *));
		if (new_destructors)
			E->destructors = new_destructors;
		else
			printf("Whoops, running out of memory.\n");

		void **new_userdatas = crealloc(E->userdatas, num_component_types * sizeof(void *), cmax * sizeof(void *));
		if (new_userdatas)
			E->userdatas = new_userdatas;
		else
			printf("Whoops, running out of memory.\n");

		struct ecs_component_init_params **new_init_params = crealloc(E->init_params, num_component_types * sizeof(struct ecs_component_init_params *), cmax * sizeof(struct ecs_component_init_params *));
		if (new_init_params)
			E->init_params = new_init_params;
		else
			printf("Whoops, running out of memory.\n");
	}
}

//Returns ctype
uint32_t ecs_component_register(ecs_ctx *E, size_t num, size_t size)
{
	if (E->free_component_slots.num == 0)
		ecs_realloc(E, ecs_entities_max(E), ecs_components_max(E) * 2);

	uint32_t ctype;
	mempool_pop(&E->free_component_slots, &ctype);

	E->components[ctype] = hmempool_new_unmanaged(num, size, ecs_entities_max(E));
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
	if (p->init)
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

void ecs_component_unregister(ecs_ctx *E, uint32_t ctype)
{
	//Remove that component from any entity that has it.
	struct hmempool *components = &E->components[ctype];
	uint32_t num = components->pool.num;
	for (int i = 0; i < num; i++) {
		//Iterate over entity indices, retrieve live entity handles.
		uint32_t eid = components->itoh[i];
		ecs_call_destructor(E, eid, ctype, hmempool_get(&E->components[ctype], eid));
	}
	if (E->init_params[ctype] && E->init_params[ctype]->deinit)
		E->init_params[ctype]->deinit(E->init_params[ctype]);
	E->init_params[ctype] = NULL;
	E->constructors[ctype] = NULL;
	E->destructors[ctype] = NULL;
	E->userdatas[ctype] = NULL;

	hmempool_delete(&E->components[ctype]);
	mempool_add(&E->free_component_slots, &ctype);
}

void * ecs_components(ecs_ctx *E, uint32_t ctype, size_t *num)
{
	//You can't safely access the list without knowing its length, so error if num is NULL.
	if (E->components[ctype].allocated && num) {
		*num = E->components[ctype].pool.num;
		return E->components[ctype].pool.pool;
	}
	return NULL;
}

const uint32_t * ecs_component_itoh(ecs_ctx *E, uint32_t ctype)
{
	return E->components[ctype].itoh;
}

uint32_t ecs_entity_add(ecs_ctx *E)
{
	if (ecs_entities_num(E) >= ecs_entities_max(E))
		ecs_realloc(E, ecs_entities_max(E) * 2, ecs_components_max(E));

	//At this point, we know there must be at least one free slot.

	//For now, eids are just hmempool handles.
	//Later, I can bitwise-or in more information to the handle, if needed.
	uint32_t h = 0;
	mempool_pop(&E->free_eids, &h);
	E->eid_used[h] = true;
	return h;
}

void ecs_entity_remove(ecs_ctx *E, uint32_t eid)
{
	//Remove all components under that handle
	for (int i = 0; i < ecs_components_max(E); i++) {
		//If it's a valid component handle, remove and clear it.
		void *component = hmempool_get(&E->components[i], eid);
		if (component) {
			ecs_call_destructor(E, eid, i, component);
			hmempool_unclaim(&E->components[i], eid);
		}
	}

	E->eid_used[eid] = false;
	mempool_add(&E->free_eids, &eid);
}

void * ecs_entity_add_component(ecs_ctx *E, uint32_t eid, uint32_t ctype)
{
	//Get the component list, resize if needed.
	struct hmempool *cl = &E->components[ctype];
	if (cl->pool.num >= cl->pool.max)
		hmempool_resize(&E->components[ctype], cl->pool.max * 2);

	hmempool_claim_raw(cl, eid);
	//Attach component to entity.
	return hmempool_get(cl, eid);
}

void * ecs_entity_add_construct_component(ecs_ctx *E, uint32_t eid, uint32_t ctype)
{
	void *c = ecs_entity_add_component(E, eid, ctype);
	if (E->constructors[ctype])
		E->constructors[ctype](eid, c, E->userdatas[ctype]);
	return c;
}

void * ecs_entity_add_copy_component(ecs_ctx *E, uint32_t eid, uint32_t ctype, void *c)
{
	//Get the component list, resize if needed.
	struct hmempool *cl = &E->components[ctype];
	if (cl->pool.num >= cl->pool.max)
		hmempool_resize(&E->components[ctype], cl->pool.max * 2);

	hmempool_claim(cl, eid, c);
	//Attach component to entity.
	return hmempool_get(&E->components[ctype], eid);
}

void * ecs_entity_add_construct_copy_component(ecs_ctx *E, uint32_t eid, uint32_t ctype, void *c)
{
	void *tmp = ecs_entity_add_copy_component(E, eid, ctype, c);
	if (E->constructors[ctype])
		E->constructors[ctype](eid, tmp, E->userdatas[ctype]);
	return tmp;
}

void ecs_entity_remove_component(ecs_ctx *E, uint32_t eid, uint32_t ctype)
{
	struct hmempool *cl = &E->components[ctype];
	void *c = hmempool_get(cl, eid);
	if (c) {
		ecs_call_destructor(E, eid, ctype, c);
		hmempool_unclaim(cl, eid);
	}
}

void ecs_entity_remove_components(ecs_ctx *E, uint32_t eid)
{
	//Simple solution for now, can optimize a bit later.
	for (int i = 0; i < ecs_components_max(E); i++)
		if (E->components[i].allocated)
			ecs_entity_remove_component(E, eid, i);
}

void * ecs_entity_get_component(ecs_ctx *E, uint32_t eid, uint32_t ctype)
{
	return hmempool_get(&E->components[ctype], eid);
}
