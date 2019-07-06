#include "universe_scene.h"
#include "datastructures/ecs.h"
#include "glla.h"
#include "math/bpos.h"
#include "macros.h"
#define ECS_ACTIVE_ECS E
#define ECS_ACTIVE_CTYPES (&ecs_ctypes)
#include "components/components_generic.h"
SCENE_IMPLEMENT(universe);

struct game_scene universe_scene;
static float screen_width;
static float screen_height;
static struct entity_ctypes ecs_ctypes = {0};
static ecs_ctx e = {.user_ctypes = &ecs_ctypes};
static ecs_ctx *E;

// struct ecs_component_init_params {
// 	size_t num, size;
// 	ecs_c_constructor_fn construct;
// 	ecs_c_destructor_fn destruct;
// 	void *userdata;
// 	uint32_t *ctype;
// 	ecs_ctx *E;
// 	void *ctx;
// 	struct ecs_component_init_params *ip;
// 	void *(*init)(struct ecs_component_init_params *);
// 	void (*deinit)(struct ecs_component_init_params *);
// };
static struct ecs_component_init_params component_init_params[] = {
//    num, size, constructor, destructor, userdata, ctype out, ECS out, init params out, init, deinit
	{.num = 10, .size = sizeof(PhysicalTemp), .ctype = &ecs_ctypes.physical},
	{.num = 5,  .size = sizeof(Camera), .ctype = &ecs_ctypes.camera},
	{.num = 500, .size = sizeof(CustomDrawable), .ctype = &ecs_ctypes.customdrawable},
	// {10, sizeof(Universal), NULL, NULL, NULL, NULL, NULL, component_universal_init, 0, NULL},
};

static uint32_t entity_star_box_new()
{
	return 0;
}

//Position is irrelevant at the scale of a galaxy
static uint32_t entity_galaxy_new(mat3 orientation, bpos_origin origin)
{
	uint32_t galaxy = entity_new();
	// entity_add_component(galaxy, (Label){.name = "Galaxy", .description = NULL});
	// entity_add_component(galaxy, (Physical){});
	// entity_add_component(galaxy, (Universal){});
	// entity_add_component(galaxy, (GridExplorable){});
	// entity_add_component(galaxy, (DistanceHysteresisExplorable){});
	// entity_add_component(galaxy, (ParentOf27){});
	// entity_add_component(galaxy, (LOD){});
	return galaxy;
}

static uint32_t entity_universe_new()
{
	uint32_t universe = entity_new();
	// entity_add_component(universe, (Label){.name = "Universe", .description = NULL})
	entity_add(universe, (PhysicalTemp){.position = (amat4)AMAT4_IDENT});
	// entity_add_component(universe, (Universal){});
	// entity_add_component(universe, (GridExplorable){});
	// entity_add_component(universe, (ParentOf27){});
	// entity_add_component(universe, (LOD){});
	//Handle big stars somehow
	//Handle dust clouds, asteroids, nebulas
	//LOD component creates/destroys 
	return universe;
}

static uint32_t entity_camera_new()
{
	uint32_t camera = entity_new();
	entity_add(camera, (PhysicalTemp){.position = (amat4)AMAT4_IDENT});
	entity_add(camera, (Camera){});

	return camera;
}

int universe_scene_init()
{
	e = ecs_new(10000, 50);
	ecs_ctx *E = &e;

	for (int i = 0; i < LENGTH(component_init_params); i++) {
		ecs_component_init(E, &component_init_params[i]);
	}

	//Universe entity will spawn and manage galaxy entities,
	uint32_t universe = entity_universe_new();
	//which spawn and manage star entities (and other stellar phenomenon like nebulas, rogue planets, space rocks), which spawn and manage planet entities
	//Then I need a spaceship/player entity, and a camera entity

	uint32_t player = entity_new();
	uint32_t camera = entity_camera_new();

	return 0;
}
void universe_scene_deinit()
{
	ecs_free(E);
}
void universe_scene_update(float dt)
{

}
void universe_scene_render()
{

}
void universe_scene_resize(float width, float height)
{

}