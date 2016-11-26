#ifndef ENTITY_H
#define ENTITY_H

#include <glalgebra.h>
#include "../drawable.h"

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

typedef struct physical_component {
	float bounding_sphere; //Stored as radius squared, centered on position.t
	amat4 position;
	amat4 velocity;
} Physical;

typedef struct entity {
	Drawable *drawable;
	Physical *physical;
	//Occludable
	//Occludent
} Entity;

Entity global_entities[512]; //Later this can be an expandable arraylist.
int num_global_entities = 0;

#endif