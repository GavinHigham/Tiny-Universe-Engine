#include "entity.h"
#include "drawable.h"
#include "physical.h"
#include "controllable.h"
#include "scriptable.h"
#include "../input_event.h" //axes

//Just picked a big number for now.
#define NDRAWABLES     2048
#define NPHYSICALS     2048
#define NCONTROLLABLES 2048
#define NSCRIPTABLES   2048

Entity global_entities[512]; //Later this can be an expandable arraylist.
uint16_t num_global_entities = 0;
uint16_t drawable_entity_indices[NDRAWABLES];
uint16_t physical_entity_indices[NPHYSICALS];
uint16_t controllable_entity_indices[NCONTROLLABLES];
uint16_t scriptable_entity_indices[NSCRIPTABLES];
Physical physical_components[NPHYSICALS];
Drawable drawable_components[NDRAWABLES];
Controllable controllable_components[NCONTROLLABLES];
Scriptable scriptable_components[NSCRIPTABLES];
uint16_t ndrawables = 0, nphysicals = 0, ncontrollables = 0, nscriptables = 0;

Entity * entity_new(uint16_t component_mask)
{
	uint16_t entity_index = num_global_entities++;
	Entity *e = &global_entities[entity_index];

	//For now, do nothing: I want to make it harder to initialize a thing without
	//a necessary function pointer (which would cause a crash).
	if (component_mask & DRAWABLE_MASK) {
		drawable_entity_indices[ndrawables] = entity_index;
		e->drawable = &drawable_components[ndrawables++];
		//Probably want some sensible default appearance here.
	}
	if (component_mask & PHYSICAL_MASK) {
		physical_entity_indices[nphysicals] = entity_index;
		e->physical = &physical_components[nphysicals++];
		//Probably want some sensible default values here.
		*e->physical = (Physical) {
			.acceleration = AMAT4_IDENT,
			.velocity     = AMAT4_IDENT,
			.position     = AMAT4_IDENT,
			.origin       = {0, 0, 0},
		};
	}
	if (component_mask & CONTROLLABLE_MASK) {
		controllable_entity_indices[ncontrollables] = entity_index;
		e->controllable = &controllable_components[ncontrollables++];
		e->controllable->control = noop_control;
		e->controllable->context = NULL;
	}
	if (component_mask & SCRIPTABLE_MASK) {
		scriptable_entity_indices[nscriptables] = entity_index;
		e->scriptable = &scriptable_components[nscriptables++];
		e->scriptable->script = noop_script;
		e->scriptable->context = NULL;
	}

	num_global_entities++;
	return e;
}

void entity_reset()
{
	num_global_entities = 0, ndrawables = 0, nphysicals = 0, ncontrollables = 0, nscriptables = 0;
}

Controllable * controllable_new(uint16_t entity_index)
{
	controllable_entity_indices[ncontrollables] = entity_index;
	return &controllable_components[ncontrollables++];
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
		components[i].control(input, nes30_buttons, &global_entities[controllable_entity_indices[i]]);
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
	entity_update_physical_components(physical_components, nphysicals);
	entity_update_drawable_components(drawable_components, ndrawables);
	entity_update_controllable_components(controllable_components, ncontrollables);
	entity_update_scriptable_components(scriptable_components, nscriptables);
}
