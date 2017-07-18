#ifndef ENTITY_H
#define ENTITY_H
#include "inttypes.h"

/*

- Entity-Component Concept -

An "Entity" is for the most part a set of references to components. We can model different object-like things in the
game by creating an Entity with the right set of components.

For example, an "enemy" entity might have the following set of components:
	Damageable - This enemy should take damage when attacked.
	Collidable - This enemy collides with other entities in the game world.
	Intelligent - This enemy observes the world around it and makes decisions based on that information.

If we want to make a variant of the enemy, we can simply modify its set of components. Adding an "Explosive" component 
might make an enemy that explodes once it has taken enough damage.

- Technical Details -

All components of a given type are stored in a single array together. This gives a high degree of memory locality
to the update process for any given type of component. When a component is removed from an entity, the last component in
 the respective component array takes its place, insuring that iteration over the entire array is simple and fast.

An entity should retain the same memory location for its entire lifetime, so that it may be referenced by the components
 of other entities. Thus, the entity array will have "holes". Storing a pointer to an entity's component is not 
recommended, as the component arrays will be shuffled around over time with the creation and deletion of components. 
Since components correct the references of their containing entities when they are deleted or created, storing a pointer
 to the entity is a better way to access a particular component over its lifetime.

- Naming Conventions -

Components should be named as adjectives, referring to properties of the entity they are attached to. For example,
an entity with a "Damagable" component is damagable. This makes code that checks for properties of an entity read
nicely, for example:
if (entity.damagable)
	damage(entity);
	
*/

typedef struct drawable_component Drawable;
typedef struct controllable_component Controllable;
typedef struct scriptable_component Scriptable;
typedef struct physical_component Physical;
typedef struct collidable_component Collidable;

enum {
	DRAWABLE_BIT     = 1,
	PHYSICAL_BIT     = 2,
	CONTROLLABLE_BIT = 4,
	SCRIPTABLE_BIT   = 8,
	COLLIDABLE_BIT   = 16,
};

typedef struct entity {
	unsigned char is_alive : 1;
	Drawable *Drawable;
	Physical *Physical;
	Controllable *Controllable;
	Scriptable *Scriptable;
	Collidable *Collidable;
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