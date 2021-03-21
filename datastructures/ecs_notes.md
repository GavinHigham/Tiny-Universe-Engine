# ECS

Rough notes on new entity component system implementation

Components are just data (fixed size)
Entities are just sets of components.
Systems operate over sets of components, leveraging a separate component-to-entity mapping to find other needed components attached to the same entity.
	Or maybe they just iterate over all entities, and operate on those that have all the components they care about? Can cache lists of entity handles that apply.

Mapping from component to entity:

	Entity * ecs_c_to_entity(ecs_ctx *E, int component_id, int component_index);
	int ecs_c_to_eid(ecs_ctx *E, int cid, int component_index);

Just need a second memory pool of pointers or indexes of containing Entity, make sure component pool operations are also done on the pointer
(Should this be optional behavior? Could be convenient to allow embedded pointers for components that are expected to only be updated in conjunction with other components.)

ECS will take an entity id and return a pointer to a list of component indexes (or pointers)
ECS helper can return a struct that's defined to contain pointers to all compile-time components, for convenient C access. Could look like this:

	#include "ecs/ecs.h"
	#include "ecs/ecs_c.h"
...

	Physical *p = ecs_entity_c(E, eid).physical;
	RuntimeComponent *r = ecs_entity(E, eid, cid); //Only while porting Lua component to C component, or accessing entity from Lua.

//Add a new component type to the ecs. Provide the size of the component, ecs will allocate storage for it and create a lookup table mapping back to the entity.
//
ecs_add_component
ecs_add_system

*********

Doubts:
	Do I really need an entity "pool"?
		Lets me iterate over all entities quickly
		Does let me add entities without O(n) search for free entity slot (or linked list of free slots)
			But I do need an O(n) search for a free handle, don't I?
				Small example:
				Handles: {0...0, 1...1}
				eid_to_entity: {0, 1}
				Entity 0 is deleted, entity 1 moved to its slot
				eid_to_entity: {-1, 0}
				New entity, handle 2...0 (how do I find out it gets handle slot 0?)
					Pool of free handle slots?
				eid_to_entity: {1, 0}
	Do I really need a component pool "pool"?
		Very few component types, even an O(n) search for a free cid is cheap
		Adding a new component type is extremely rare
		Do I even even iterate over all component pools?
			Component systems could handle that themselves
		Can even have a pool of free cids...
Conclusions, after sleeping on it:
	Array of entity handle slots
		Pool (used as a stack) of free entity slots
	Array of cid slots
		Pool (used as a stack) of free cid slots

Later update:
	Made a "handled mempool" datastructure, simplifies things a lot. Just have:
		hmempool of entities, which are arrays of handles to components, where position in the array
			is implicitly the component type id.
		Array of hmempools for each component type, indexed by component type id.
		mempool of free component slots, so I can add/remove component types at runtime.

With this implementation, accessing a component goes roughly like this,
using eid and ctype:

    cid = entities.pool[entities.slots[eid]][ctype]
    component_pool = components[ctype]
    component = component_pool.pool[component_pool.slots[cid]]

Essentially 6 table lookups, completely serial, plus some arithmetic. Once the component handle list is accessed (```entities.pool[entities.slots[eid]]```), seeing if an entity has certain components on it is a single lookup.

## Performance improvement if I need it:

If I can make the slots table for each component hmempool big enough to be indexed by any eid, I can instead do this:

	component_pool = components[ctype]
	component = component_pool.pool[component_pool.slots[eid]]

Only 3 table lookups to get a component from an entity, though it takes those 3 every time you want to check if an entity has a component. Can speed it up by storing a bitmask per entity of which components are attached. Also, when iterating over one component type, itoh[i] will give the eid, which can be used to look up sibling components somewhat faster.


*********

There are three kinds of reallocation I need to be concerned with:
	Resize entity pool:
		realloc entity pool
		realloc entity-handle-to-entity table
		update all component-to-entity pointers?
			Could use an index or entity handle instead to avoid this
				Just use a function for now, I can change how I accomplish the mapping later
		realloc free_eids pool

	Resize component pool pool
		realloc component pool pool
		realloc cid_to_component_pool
		realloc entity pool (each entity is now bigger, can't just do realloc or memcpy!)
		realloc free_cids pool

	Resize individual component pool
		realloc component pool
		realloc component-to-entity table
		update pointers in entity pool
			If I use indexes in entities instead, I can avoid this

There are three kinds of "pool item moved" situations I need to be concerned with:
	Entity deleted, entities[num] entity moved to deleted slot
		All components attached need to be deleted as well
		All components with pointer to entities[num] need to be updated
			Could just use stable entity handle to avoid this
				Pay slightly less in management cost (memory always, CPU when entities are destroyed)
				Pay slightly more in the hot path, component to parent entity has a level of indirection
					Just use a function for now, I can change how I accomplish the mapping later
		eid_to_entity entry invalidated
			eid_to_entity slot added to free_eids pool

	Component type deleted
		All entities with that component need to have it invalidated
		cid_to_component_pool[deleted] needs to be updated/invalidated
			free cids pool can have that slot added
		Component pool needs to be deleted
		Component-index-to-entity-index pool needs to be deleted

	Component deleted, component<type>[num] component moved to deleted slot
		Entity with that component needs to be updated (may have been already)
		Component-index-to-entity-index pool should be updated (applying the same remove operation on it will work)
		Entity with moved component needs to point to new index


*********

I should start eids and cids at 1, so that 0 can be an invalid handle.
Might be able to come up with a clever bit of math that can make using an invalid handle return NULL without using a branch.

Assuming an eid has two parts, generation and slot (probably using bitmasks, but illustrated here using a struct for simplicity)

	uint32_t slot = 0;
	struct _eid {
		uint16_t generation;
		uint16_t slot;
	} eid = {0, 1}; //0 slot is considered invalid.
	slot = eid.slot; //Simple+fast way, does not catch using invalidated handles
	slot = (eid.generation == entities.generations[eid.slot]) ? eid.slot : 0; //Redirects invalid handles to the 0-slot.
	slot = ((eid.generation != entities.generations[eid.slot]) - 1) & eid.slot; //Same as previous, but branchless.

There we go :)

*********


Skip "ecs_register_system" for now, systems can just iterate over the component pools themselves. Can integrate back into the ECS later.

Example code:

```
//Struct to hold all the info needed to initialize a particular component's runtime.
struct game_component {
	size_t num, size;
	ecs_c_constructor_fn construct;
	ecs_c_destructor_fn destruct;
	void (init)(uint32_t);
	void *ctx;
}

#define ECS_ACTIVE_ECS E
#include "entity_generic.h"

...

ecs_ctx e = ecs_new(10000, 50);
ecs_ctx *E = &e;

for (int i = 0; i < game_components; i++) {
	struct game_component g = game_components[i];
	uint32_t ctype = ecs_register_component(E, g.num, g.size);
	g.ctx = g.init(ctype);
	ecs_component_set_construct_destruct(E, ctype, g.construct, g.destruct, g.userdata);
}

uint32_t make_missile(ecs_ctx *E, vec3 position, vec3 velocity, uint32_t lifetime)
{
	uint32_t missile = entity_new();
	entity_add_component(missile, (Physical){
		.position = position, .velocity = velocity,
		.mass = missile_mass, .moment = missile_moment});
	entity_add_component(missile, (Doomed){.deathtime_ms = lifetime});
	entity_add_component(missile, missile_drawable);
	entity_add_component(missile, (Damageable){.health = 50});
	entity_add_component(missile, (Fragile){.bump_damage = 50});
	return missile;
}

uint32_t make_homing_missile(ecs_ctx *E, vec3 position, vec3_velocity, uint32_t lifetime, uint32_t homing_flags, float torque, uint32_t target)
{
	uint32_t missile = make_missile(E, position, velocity, lifetime);
	entity_add_component(missile, (Homing){.target = target, .homing_flags = homing_flags, .torque = torque});
}

```

Example Lua code:

```
e = ecs_new(10000, 50)

--Example of initializing C components from Lua, with syntactic sugar. Initializing Lua components should be even easier.
for i,v in ipairs(components) do
	--Will register the component type, store the ctype handle to the component type instance, and initialize the component type runtime.
	v:init(e)
end

...

function SpaceGame:make_missile(position, velocity, lifetime)
	local missile = self.entity.new()
	missile:add_components({
		Physical = {
			position = position, velocity = velocity,
			mass = missile_mass, moment = missile_moment
		},
		Doomed = {deathtime_ms = lifetime},
		Damageable = {health = 50},
		Fragile = {bump_damage = 50},
		Drawable = missile_drawable
	})
	return missile
end

function SpaceGame:make_homing_missile(position, velocity, lifetime, homing_flags, torque, target)
	local missile = SpaceGame:make_missile(position, velocity, lifetime)
	missile:add_component("Homing", {target = target, homing_flags = homing_flags, torque = torque})
	return missile
end

```

Component ideas:

		DebugDrawable component
			XRay mode
				Outline view - geometry shader
				Rimlit view - fragment shader / compositing
			Can show bounding volumes, relationships, etc.
			DebugLines component
				Holds a bunch of lines (or limit to some number?) that can show relationships between entities for debugging purposes.
				Can have draw over everything, draw against depth (must consider logarithmic depth), configurable color.
				Consider just one debug line per entity? Can create transient entity for multiple?
		Components for Drawable:
			TextureNormalMapped
			VertexNormalMapped
			((Texture)|(Vertex))<SurfaceProperty>Mapped
			Can have functions to default-generate some if required others are present
			TexturePositionMapped? Texture becomes definition of 3D volume, vertices are now quantization of surface. Maybe makes some cool "cyberspace" effects for creating / destroying, or things shimmering over the surface. Possible way to generate LODs? Similar in concept to the direction I want to take planet generation.
			VertexUVMapped
				Only one UV map per entity, hopefully not a terrible limitation
		DebugUI component
			Can change aspects of DebugDrawable, can add other controls for testing
		Pickable component
			Configurable pick mode
				Draw / read-back mode
				Bounding volume / mesh walk
		SwingControllable
			Locks distance to ((camera)|<arbitrary entity>), position on-screen directly
		Dump entity
			Serialize entity (including binary if any components are not serializable)
			Useful for debug and savestates
		Doomed component
			Stores a tick count or time at which the entity will be destroyed
		DeathEventHandler component
			Script or function that's invoked as the entity is dying (after "death", before deallocation. This system collects garbage every frame.)
		DeathPact
			Entity will die if some other entity is dead.
		<Event>EventHandler component
			Script or function that's invoked in response to various events?
		Universal component
			Signifies that the entity does not go away if unobserved
			Ex: the universe, other players, pursuing AI, player base if any, important player changes, valuable player items
		Observer component
			Signifies that non-universal entities should continue existing in its observed area. On creation should iterate over universal components to create any observable phenomenon expected at its position? (ex: "Universe" entity is universal, responsible for creating galaxy entities, which create star entities, which create planet entities)
			Should also be universal to prevent them being immediately culled?
			Can provide list of observed entities
		RemoteObserver
			Can observe a single entity from anywhere, basically makes observed entity universal.
		ShaderPipeline
			Contains geometry, vertex and fragment shaders, as well as the linked program - refcounted so it can be shared?
			Contains information about inputs/outputs? Could parse out of the shaders / snippets for validation.
		InstanceDrawable
			Entity will simply be a copy of a common shared entity, with a different position.
		BatchDrawable
			Like InstanceDrawable, but maybe allows more customization per instance?


# Renderer System

Iterates over all drawable / customdrawable / batchdrawable components
Maintains internal state to reduce copying over multiple frames?
Sort drawables by "sort key" which can encompass bound shaders / buffers / textures, depth
	Can use a different sort key on other platforms (such as mobile which uses tiled deferred rendering and thus doesn't benefit as much from depth sorting)

How do I want to do cubemaps and textures? Specifically, do I want entities to support multiple? Do I want them to be able to render to and render from? Do I want textures to be refcounted and shared between entities?
	No simple way to do multiple textures on a single entitity, it's the "implement an inventory" problem all over again
	If I have a limit to the number of textures per entitity, I can make a component that wraps that many.
	Could have a "refcount" component that will destroy entity if it reaches 0 - would be useful if I make textures components of entities.
	Textures can be directly attached to entities (if entity only has / needs one, and it's not shared)
		Otherwise, entity should have a TextureReferenceN? component, which allows rendering system to lookup the refcounted textures (and deref if the entity is destroyed)
		Drawable component will look at attached texture or TextureReference and bind before drawing if configured to do so
		Work with CustomDrawable for now, identify patterns, and design Drawable to fit a large number of the possibilities

CustomDrawables will set every uniform / bit of OpenGL state they need before being rendered
BatchDrawables will have BatchStart / BatchDraw / BatchEnd calls

Should I switch to something like sokol_gfx instead of OpenGL?

# Constructors / Destructors

The original implementation had constructor and destructor functions to clean up data associated with a component if it's removed (For example, may want to clean up textures / meshes when refcount hits 0).
I found that this made the implementation more complicated, and I didn't want to have to check for a constructor / destructor every single time I created or destroyed an entity - I want that to be extremely fast (meaning branchless). I am considering using construction and destruction functions, but that still means special-casing removing components from an entity or deleting the entity. The complexity remains.

I just had the idea that I could implement a simple garbage collector. Periodically (every frame? when I run out of some resource?) a system can scan its list of components and determine which resources are no longer claimed, and recycle them. The systems should each maintain a list of entities handles that own any system (in the ECS system sense) resource. Since the number of live components of any type is known at all times, the scan can be skipped if the difference between the number of live components and the number of allocations made on behalf of those components is not very high.

# Systems
Each system exposes an API, it need not be standard - init, deinit, update are likely common. Drawable or CustomDrawable will have render or draw. The API calls should take a system context pointer, which could be stored in the ECS or the scene.

# Procedural exploration
	I need a way to:
		spawn things when I enter an area and despawn them when I leave
			Galaxies, stars - can use starbox strategy if I'm okay with cubic grid
		not recreate them on top of each other (don't want to spawn the same planet twice, spawn trees inside each other etc.)
			Hash based on position, hash lookup on creation?
			Chunking sort of works, but would be nice to be more granular
		"reset" them (defeated enemies go away and are respawned)
			Enemies / corpses can have DeathPact component to spawn area, destroy and recreate spawn area?
				Same but DeathPact to enemy "leader"?

# C vs. Lua Scripting
Entities should be able to respond to events by registering handlers, these need to be put somewhere - individual components, or a component that can register several, etc.

From the engine side, optional constructors/destructors, and "Scriptable" for logic that runs every logic tick.

From the scripting side, onTick, onDraw, onDestroy, onCollide, etc. using the listener pattern