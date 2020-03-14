#include "test/test_main.h"
#include "datastructures/ecs.h"
#include <string.h>
#include <stdlib.h>

uint32_t component_description;//, component_destroyable, component_destroy, component_damaging;

uint32_t ecs_test_create_with_description(ecs_ctx *E, char *desc)
{
	uint32_t tmp = ecs_entity_add(E);
	*(char **)ecs_entity_add_component(E, tmp, component_description) = desc;
	return tmp;
}

void ecs_test_strdup_constructor(uint32_t eid, void *component, void *userdata)
{
	char **component_str = component;
	*component_str = strdup(*component_str);
	(*(uint32_t *)userdata)++;
}

void ecs_test_free_destructor(uint32_t eid, void *component, void *userdata)
{
	free(*(void **)component);
	*(void **)component = NULL;
	(*(uint32_t *)userdata)--;
}

int ecs_test_1()
{
	int nf = 0; //Number of failures
	ecs_ctx e = ecs_new(5, 1);
	ecs_ctx *E = &e;
	char *descriptions[] = {
		"Fluffy",
		"Pointy",
		"Turgid",
		"Smelly",
		"Yeet",
	};
	size_t num = LENGTH(descriptions);
	uint32_t component_description = ecs_component_register(E, num, sizeof(char *));
	uint32_t entities[2*num];

	//Add "num" entities with the "component_description" component attached.
	for (int i = 0; i < num; i++)
		entities[i] = ecs_test_create_with_description(E, descriptions[i]);

	TEST_SOFT_ASSERT(nf, ecs_entities_max(E) == ecs_entities_num(E))

	//Check that the descriptions are matching.
	for (int i = 0; i < num; i++) {
		// printf("%u: %s vs %s\n", entities[i], *(char **)ecs_entity_get_component(E, entities[i], component_description), descriptions[i]);
		TEST_SOFT_ASSERT(nf, !strcmp(*(char **)ecs_entity_get_component(E, entities[i], component_description), descriptions[i]))
	}

	uint32_t strdup_allocations = 0;
	//Register "dynamic_component_description" component, which makes and stores a copy of the description.
	//An ECS realloc will occur to make room for this new component type.
	uint32_t dynamic_component_description = ecs_component_register(E, num, sizeof(char *));
	//This component calls strdup on construction, and free on destruction.
	ecs_component_set_construct_destruct(E, dynamic_component_description,
		ecs_test_strdup_constructor, ecs_test_free_destructor, &strdup_allocations);

	//An ECS realloc will happen with the first of these, to make room for "num+1" entities.
	for (int i = num; i < 2*num; i++)
		entities[i] = ecs_entity_add(E);

	//Attach the dynamic description to the last "num" entities.
	for (int i = num; i < 2*num; i++)
		ecs_entity_add_construct_copy_component(E, entities[i], dynamic_component_description, &descriptions[i/2]);

	//check number of strdup allocations
	TEST_SOFT_ASSERT(nf, strdup_allocations == num);

	//Check that the descriptions are matching.
	for (int i = num; i < 2*num; i++) {
		// printf("%u: %s vs %s\n", entities[i], *(char **)ecs_entity_get_component(E, entities[i], component_description), descriptions[i]);
		TEST_SOFT_ASSERT(nf, !strcmp(*(char **)ecs_entity_get_component(E, entities[i], dynamic_component_description), descriptions[i/2]))
	}

	//Remove an entity
	ecs_entity_remove(E, entities[num]);
	//Check that the component was deallocated
	TEST_SOFT_ASSERT(nf, strdup_allocations == num - 1);
	//Remove a component
	ecs_entity_remove_component(E, entities[num+1], dynamic_component_description);
	//Check that the component was deallocated
	TEST_SOFT_ASSERT(nf, strdup_allocations == num - 2);
	//Unregister the component type
	ecs_component_unregister(E, dynamic_component_description);
	//Check that all remaining components of that type were deallocated
	TEST_SOFT_ASSERT(nf, strdup_allocations == 0);

	ecs_free(E);

	return nf;
}