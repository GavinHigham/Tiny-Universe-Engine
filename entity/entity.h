#ifndef ENTITY_H
#define ENTITY_H
#include "inttypes.h"

/* Entity-Component Design

Entities are a tough concept to define explicitly. An entity instance is defined simply by the
components it possesses. For example, an "enemy" entity might have a "health" component, which a "damage"
component would check for if the two entities collide. The presence of an "AI" component would differentiate
between a damageable piece of scenery and a moving, intelligent creature. Since any entity can have any component,
we can easily handle interactions between vast numbers of possible entities, without entering inheritance hell.

All components of a given type are stored in a single array together. This gives a high degree of memory locality
to the update process for any given type of component.

Components should be named as adjectives, referring to properties of the entity they are attached to. For example,
an entity with a "Damagable" component is damagable. This makes code that checks for properties of an entity read
nicely, for example:
if (entity.damagable)
	damage(entity);

An entity instance can be used as the blueprint for other instances, as if instantiating from a class. The entity
that other entities are copied from would be known as a "prefab", "prototype", or "blueprint" in this case.

*/

typedef struct drawable_component Drawable;
typedef struct controllable_component Controllable;
typedef struct scriptable_component Scriptable;
typedef struct physical_component Physical;

enum {
	DRAWABLE_BIT     = 1,
	PHYSICAL_BIT     = 2,
	CONTROLLABLE_BIT = 4,
	SCRIPTABLE_BIT   = 8,
};

typedef struct entity {
	Drawable *Drawable;
	Physical *Physical;
	Controllable *Controllable;
	Scriptable *Scriptable;
	//Occludable
	//Occludent
	//Parent_transform? Could create a chain of transforms.
} Entity;

extern Entity global_entities[512]; //Later this can be an expandable arraylist.
extern uint16_t num_global_entities;

//Creates a new entity and any associated components specified in component_mask.
Entity * entity_new(uint16_t component_mask);
//Deletes an entity and all associated components.
void entity_delete(Entity *);
void entity_reset();
void entity_update_components();

#endif