#include <assert.h>
#include "entity.h"
#include "drawable.h"
#include "physical.h"
#include "controllable.h"
#include "collidable.h"
#include "scriptable.h"
#include "emissive.h"
#include "breadcrumber.h"
#include "../input_event.h" //axes

//To work around C's lack of generic functions, this file uses the "X-Macros" pattern,
//where a macro is defined, a file using that macro multiple times with different arguments is included,
//and the macro is undefined. This makes it fairly straightforward to generate generic specializations.

//Just picked a big number for now.
#define ENTITIES_MAX      512

//Define the max elements for each component array.
#define ENTITY_COMPONENT_GEN(Capitalized, uncapitalized, count) \
const unsigned int uncapitalized##_max = count ;
#include "all_components.h"
#undef ENTITY_COMPONENT_GEN

Entity global_entities[ENTITIES_MAX]; //Later this can be an expandable arraylist.
uint16_t first_free_index = 0;
uint16_t num_global_entities = 0;

/*
For each kind of component, generate:
	1. A component-to-entity-index lookup table
	2. A component count
	3. An array for all components of that type.
*/
#define ENTITY_COMPONENT_GEN(Capitalized, uncapitalized, count)     \
	uint16_t uncapitalized##_entity_indices[count];                 \
	uint16_t uncapitalized##_count = 0;                             \
	Capitalized uncapitalized##_components[count];
#include "all_components.h"
#undef ENTITY_COMPONENT_GEN

/*
Generate the entity_make_<component name> functions for components with generated make/unmake functions.

This macro generates a function that:
	1. Sets an entry in the component-to-entity-index lookup table
	2. Sets the entity's component to a new component from the correct component array.
	3. Initializes the component from the passed-in argument.
	4. Sets the component bit in the entity.
*/
#define ENTITY_GEN_MAKE_FN(Capitalized, uncapitalized, count, fn_prefix)              \
void fn_prefix##uncapitalized(Entity *entity, Capitalized uncapitalized)              \
{                                                                                     \
	uncapitalized##_entity_indices[uncapitalized##_count] = entity - global_entities; \
	entity->uncapitalized = &uncapitalized##_components[uncapitalized##_count++];     \
	*entity->uncapitalized = uncapitalized;                                           \
	entity->component_bits |= uncapitalized##_bit;                                    \
}
#define ENTITY_COMPONENT_GEN(...) ENTITY_GEN_MAKE_FN(__VA_ARGS__, entity_make_)
#include "generated_components.h"
#undef ENTITY_COMPONENT_GEN

/*
Generate entity_alloc_<component name> functions for components that manually define make/unmake.

The body of these functions is identical to that of the entity_make_<component name> functions,
just named entity_alloc_<component name> instead, so that the make function can be defined manually.
The make function should then call its respective alloc function at some point.
*/
#define ENTITY_COMPONENT_GEN(...) ENTITY_GEN_MAKE_FN(__VA_ARGS__, entity_alloc_)
#include "nongenerated_components.h"
#undef ENTITY_COMPONENT_GEN

/*
Generate the entity_unmake_<component name> functions for components with generated make/unmake functions.

This macro generates functions to "unmake" (delete) components owned by an entity.
	1. Overwrite the component with the last one from that component array.
	2. Update the entity index table and decrement the component count.
	3. Set the component pointer to NULL.
	4. Clear the component bit.\
*/
#define ENTITY_GEN_UNMAKE_FN(Capitalized, uncapitalized, count, fn_prefix)                                                                        \
void fn_prefix##uncapitalized(Entity *e)                                                                                                          \
{                                                                                                                                                 \
	if(e->uncapitalized) {                                                                                                                        \
		*e->uncapitalized = uncapitalized##_components[uncapitalized##_count - 1];                                                                \
		uncapitalized##_entity_indices[e->uncapitalized - uncapitalized##_components] = uncapitalized##_entity_indices [--uncapitalized##_count]; \
		e->uncapitalized = NULL;                                                                                                                  \
	    e->component_bits &= ~uncapitalized##_bit; }}

#define ENTITY_COMPONENT_GEN(...) ENTITY_GEN_UNMAKE_FN(__VA_ARGS__, entity_unmake_)
#include "generated_components.h"
#undef ENTITY_COMPONENT_GEN

/*
Generate entity_dealloc_<component name> functions for components that manually define make/unmake.

The body of these functions is identical to that of the entity_unmake_<component name> functions,
just named entity_dealloc_<component name> instead, so that the unmake function can be defined manually.
The unmake function should then call its respective dealloc function at some point.
*/
#define ENTITY_COMPONENT_GEN(...) ENTITY_GEN_UNMAKE_FN(__VA_ARGS__, entity_dealloc_)
#include "nongenerated_components.h"
#undef ENTITY_COMPONENT_GEN

//Find the first free slot in the entities array, scanning linearly.
Entity * entity_first_free()
{
	for (int i = first_free_index; i < ENTITIES_MAX; i++)
		if (!global_entities[i].is_alive)
			return &global_entities[i];
	return NULL;
}

Entity * entity_new()
{
	Entity *e = entity_first_free();
	assert(e);
	*e = (Entity){.is_alive = 1};
	num_global_entities++;
	return e;
}

void entity_delete(Entity *e)
{
/*
Run the "unmake" function for each component type.
*/
#define ENTITY_COMPONENT_GEN(Capitalized, uncapitalized, count) entity_unmake_##uncapitalized(e);
#include "all_components.h"
#undef ENTITY_COMPONENT_GEN

	e->is_alive = 0;
	uint16_t e_index = e - global_entities;
	if (e_index < first_free_index)
		first_free_index = e_index;
	num_global_entities--;
}

void entity_reset()
{
	num_global_entities = 0, drawable_count = 0, physical_count = 0, controllable_count = 0, scriptable_count = 0;
}

void entity_update_physical_components(Physical *components, uint16_t ncomponents)
{
	for (int i = 0; i < ncomponents; i++) {
		//Some kind of update function inline here
	}
}

void entity_update_drawable_components(Drawable *components, uint16_t ncomponents)
{
	for (int i = 0; i < ncomponents; i++) {
		//Some kind of update function inline here
	}
}

void entity_update_controllable_components(Controllable *components, uint16_t ncomponents)
{
	float controller_max = 32768.0;
	struct controller_axis_input input = {
		axes[LEFTX]  / controller_max,
		axes[LEFTY]  / controller_max,
		axes[RIGHTX] / controller_max,
		axes[RIGHTY] / controller_max,
		axes[TRIGGERLEFT]  / controller_max,
		axes[TRIGGERRIGHT] / controller_max,
	};

	struct mouse_input mouse;
	mouse.buttons = SDL_GetRelativeMouseState(&mouse.x, &mouse.y);

	for (int i = 0; i < ncomponents; i++) {
		components[i].control(input, nes30_buttons, mouse, &global_entities[controllable_entity_indices[i]]);
	}
}

void entity_update_scriptable_components(Scriptable *components, uint16_t ncomponents)
{
	for (int i = 0; i < ncomponents; i++) {
		components[i].script(&global_entities[scriptable_entity_indices[i]]);
	}
}

void entity_update_components()
{
	entity_update_physical_components(physical_components, physical_count);
	entity_update_drawable_components(drawable_components, drawable_count);
	entity_update_controllable_components(controllable_components, controllable_count);
	entity_update_scriptable_components(scriptable_components, scriptable_count);
}
