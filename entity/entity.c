#include <assert.h>
#include "entity.h"
#include "drawable.h"
#include "physical.h"
#include "controllable.h"
#include "collidable.h"
#include "scriptable.h"
#include "../input_event.h" //axes

//Just picked a big number for now.
#define ENTITIES_MAX      512
#define DRAWABLE_MAX     2048
#define PHYSICAL_MAX     2048
#define CONTROLLABLE_MAX 2048
#define COLLIDABLE_MAX   2048
#define SCRIPTABLE_MAX   2048

//Adds a component of given type to the given entity.
#define ENTITY_ADD_COMPONENT(entity, type) do {\
	type##_entity_indices[type##_count] = entity - global_entities; \
	entity->type = &type##_components[type##_count++]; } while(0)

//Overwrite the component with the last one from that component array,
//update the entity index table, decrement the component count,
//set component pointer to NULL.
#define ENTITY_REMOVE_COMPONENT(entity, type) while(entity->type) {\
	*entity->type = type##_components[type##_count - 1]; \
	type##_entity_indices[entity->type - type##_components] = type##_entity_indices[--type##_count]; \
	entity->type = NULL; }

//Declare all the global variables needed for a particular class of entity.
#define DECLARE_ENTITY(type, maxnum) \
	uint16_t type##_entity_indices[maxnum]; \
	uint16_t type##_count = 0; \
	type type##_components[maxnum]; \

Entity global_entities[ENTITIES_MAX]; //Later this can be an expandable arraylist.
uint16_t first_free_index = 0;
uint16_t num_global_entities = 0;

DECLARE_ENTITY(Drawable, DRAWABLE_MAX)
DECLARE_ENTITY(Physical, PHYSICAL_MAX)
DECLARE_ENTITY(Scriptable, SCRIPTABLE_MAX)
DECLARE_ENTITY(Controllable, CONTROLLABLE_MAX)
DECLARE_ENTITY(Collidable, COLLIDABLE_MAX)

Entity * entity_first_free();

Entity * entity_new(uint16_t component_mask)
{
	Entity *e = entity_first_free();//&global_entities[entity_index];
	assert(e);
	*e = (Entity){.is_alive = 1};
	num_global_entities++;

	//For now, do nothing: I want to make it harder to initialize a thing without
	//a necessary function pointer (which would cause a crash).
	if (component_mask & DRAWABLE_BIT) {
		ENTITY_ADD_COMPONENT(e, Drawable);
		//Probably want some sensible default appearance here.
	}
	if (component_mask & PHYSICAL_BIT) {
		ENTITY_ADD_COMPONENT(e, Physical);
		*e->Physical = (Physical) {
			.acceleration = AMAT4_IDENT,
			.velocity     = AMAT4_IDENT,
			.position     = AMAT4_IDENT,
			.origin       = {0, 0, 0},
		};
	}
	if (component_mask & CONTROLLABLE_BIT) {
		ENTITY_ADD_COMPONENT(e, Controllable);
		e->Controllable->control = noop_control;
		e->Controllable->context = NULL;
	}
	if (component_mask & COLLIDABLE_BIT) {
		ENTITY_ADD_COMPONENT(e, Collidable);
	}
	if (component_mask & SCRIPTABLE_BIT) {
		ENTITY_ADD_COMPONENT(e, Scriptable);
		e->Scriptable->script = noop_script;
		e->Scriptable->context = NULL;
	}

	return e;
}

void entity_delete(Entity *e)
{
	ENTITY_REMOVE_COMPONENT(e, Drawable);
	ENTITY_REMOVE_COMPONENT(e, Physical);
	ENTITY_REMOVE_COMPONENT(e, Scriptable);
	ENTITY_REMOVE_COMPONENT(e, Controllable);
	e->is_alive = 0;
	uint16_t e_index = e - global_entities;
	if (e_index < first_free_index)
		first_free_index = e_index;
	num_global_entities--;
}

void entity_reset()
{
	num_global_entities = 0, Drawable_count = 0, Physical_count = 0, Controllable_count = 0, Scriptable_count = 0;
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

	for (int i = 0; i < ncomponents; i++) {
		components[i].control(input, nes30_buttons, &global_entities[Controllable_entity_indices[i]]);
	}
}

void entity_update_scriptable_components(Scriptable *components, uint16_t ncomponents)
{
	for (int i = 0; i < ncomponents; i++) {
		components[i].script(&global_entities[Scriptable_entity_indices[i]]);
	}
}

void entity_update_components()
{
	entity_update_physical_components(Physical_components, Physical_count);
	entity_update_drawable_components(Drawable_components, Drawable_count);
	entity_update_controllable_components(Controllable_components, Controllable_count);
	entity_update_scriptable_components(Scriptable_components, Scriptable_count);
}
