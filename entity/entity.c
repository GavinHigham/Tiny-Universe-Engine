#include <assert.h>
#include "entity.h"
#include "drawable.h"
#include "physical.h"
#include "controllable.h"
#include "collidable.h"
#include "scriptable.h"
#include "emissive.h"
#include "../input_event.h" //axes

//Just picked a big number for now.
#define ENTITIES_MAX      512

//Define the max elements for each component array.
#define ENTITY_COMPONENT_DECLARE(Capitalized, uncapitalized, count) \
const unsigned int uncapitalized##_max = count ;
#include "declared_components.h"
#undef ENTITY_COMPONENT_DECLARE

//Adds a component of given type to the given entity.
#define ENTITY_ADD_COMPONENT(entity, type) do {                     \
	type##_entity_indices[type##_count] = entity - global_entities; \
	entity->type = &type##_components[type##_count++]; } while(0)

Entity global_entities[ENTITIES_MAX]; //Later this can be an expandable arraylist.
uint16_t first_free_index = 0;
uint16_t num_global_entities = 0;

//Define component-entity indices buffers, component counts, and component buffers.
#define ENTITY_COMPONENT_DECLARE(Capitalized, uncapitalized, count) \
	uint16_t uncapitalized##_entity_indices[count];                 \
	uint16_t uncapitalized##_count = 0;                             \
	Capitalized uncapitalized##_components[count];
#include "declared_components.h"
#undef ENTITY_COMPONENT_DECLARE

Entity * entity_first_free();

Entity * entity_new()
{
	Entity *e = entity_first_free();//&global_entities[entity_index];
	assert(e);
	*e = (Entity){.is_alive = 1};
	num_global_entities++;

	// //For now, do nothing: I want to make it harder to initialize a thing without
	// //a necessary function pointer (which would cause a crash).
	// if (component_mask & DRAWABLE_BIT) {
	// 	ENTITY_ADD_COMPONENT(e, Drawable);
	// 	//Probably want some sensible default appearance here.
	// }
	// if (component_mask & PHYSICAL_BIT) {
	// 	ENTITY_ADD_COMPONENT(e, Physical);
	// 	*e->Physical = (Physical) {
	// 		.acceleration = AMAT4_IDENT,
	// 		.velocity     = AMAT4_IDENT,
	// 		.position     = AMAT4_IDENT,
	// 		.origin       = {0, 0, 0},
	// 	};
	// }
	// if (component_mask & CONTROLLABLE_BIT) {
	// 	ENTITY_ADD_COMPONENT(e, Controllable);
	// 	e->Controllable->control = noop_control;
	// 	e->Controllable->context = NULL;
	// }
	// if (component_mask & COLLIDABLE_BIT) {
	// 	ENTITY_ADD_COMPONENT(e, Collidable);
	// }
	// if (component_mask & SCRIPTABLE_BIT) {
	// 	ENTITY_ADD_COMPONENT(e, Scriptable);
	// 	e->Scriptable->script = noop_script;
	// 	e->Scriptable->context = NULL;
	// }

	return e;
}

//Define the max elements for each component array.
#define ENTITY_COMPONENT_DECLARE(Capitalized, uncapitalized, count)                   \
void entity_make_##uncapitalized(Entity *entity, Capitalized uncapitalized)           \
{                                                                                     \
	uncapitalized##_entity_indices[uncapitalized##_count] = entity - global_entities; \
	entity->uncapitalized = &uncapitalized##_components[uncapitalized##_count++];     \
	*entity->uncapitalized = uncapitalized;                                           \
}
#include "declared_components.h"
#undef ENTITY_COMPONENT_DECLARE


void entity_delete(Entity *e)
{
/*
For each component:
	1. Overwrite the component with the last one from that component array.
	2. Update the entity index table and decrement the component count.
	3. Set the component pointer to NULL.
*/
#define ENTITY_COMPONENT_DECLARE(Capitalized, uncapitalized, count)                                                                           \
if(e->uncapitalized) {                                                                                                                        \
	*e->uncapitalized = uncapitalized##_components[uncapitalized##_count - 1];                                                                \
	uncapitalized##_entity_indices[e->uncapitalized - uncapitalized##_components] = uncapitalized##_entity_indices [--uncapitalized##_count]; \
	e->uncapitalized = NULL; }
#include "declared_components.h"
#undef ENTITY_COMPONENT_DECLARE

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

Entity * entity_first_free()
{
	for (int i = first_free_index; i < ENTITIES_MAX; i++)
		if (!global_entities[i].is_alive)
			return &global_entities[i];
	return NULL;
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
