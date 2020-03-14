
#include "universe_scene.h"
#include "datastructures/ecs.h"
#include "glla.h"
#include "math/bpos.h"
#include "math/utility.h"
#include "macros.h"
#include "shader_utils.h"
#include "space/star_box.h"
#define ECS_ACTIVE_ECS E
#define ECS_ACTIVE_CTYPES (&ecs_ctypes)
#include "components/components_generic.h"
#include <math.h>
#include <assert.h>
SCENE_IMPLEMENT(universe);

struct game_scene universe_scene;
uint32_t default_camera = 0;
static struct entity_ctypes ecs_ctypes = {0};
static ecs_ctx e = {.user_ctypes = &ecs_ctypes};
static ecs_ctx *E = &e;

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
	{.num = 10,  .size = sizeof(Framebuffer),    .ctype = &ecs_ctypes.framebuffer},
	{.num = 10,  .size = sizeof(PhysicalTemp),   .ctype = &ecs_ctypes.physical},
	{.num = 10,  .size = sizeof(Camera),         .ctype = &ecs_ctypes.camera,         .construct = camera_constructor},
	{.num = 500, .size = sizeof(CustomDrawable), .ctype = &ecs_ctypes.customdrawable},
	{.num = 500, .size = sizeof(Label),          .ctype = &ecs_ctypes.label,          .construct = label_constructor,   .destruct = label_destructor},
	{.num = 500, .size = sizeof(ScriptableTemp), .ctype = &ecs_ctypes.scriptable},
	{.num = 500, .size = sizeof(Target),         .ctype = &ecs_ctypes.target},
	// {10, sizeof(Universal), NULL, NULL, NULL, NULL, NULL, component_universal_init, 0, NULL},}
};

bool target_or_self_origin(uint32_t eid, bpos_origin *origin)
{
	Target *t = NULL;
	PhysicalTemp *p = NULL;
	//If it has a target, and the target is physical, use the target origin. Else, try to use self origin.
	if (((t = entity_target(eid)) && (p = entity_physicaltemp(t->target))) || (p = entity_physicaltemp(eid))) {
		*origin = p->origin;
		return true;
	}
	return false;
}

scriptabletemp_callback(entity_star_box_scriptable)
{
	bpos_origin origin = {0,0,0};
	target_or_self_origin(eid, &origin);
	star_box_update(entity_customdrawable(eid)->ctx, origin);
}

customdrawable_callback(entity_star_box_draw)
{
	Camera *c = entity_camera(camera);
	glUniform1f(effects.star_box.log_depth_intermediate_factor, c->log_depth_intermediate_factor);
	star_box_draw(entity_customdrawable(self)->ctx, entity_physicaltemp(camera)->origin, c->proj_mat);
	checkErrors("Universe %d", __LINE__);
}

CHANGEME(entity_star_box_construct)
{
	bpos_origin origin = {0,0,0};
	target_or_self_origin(eid, &origin);
	((CustomDrawable *)c)->ctx = star_box_init(star_box_new(), origin);
	return c;
}

CHANGEME(entity_star_box_destruct)
{
	star_box_deinit(((CustomDrawable *)c)->ctx);
	return NULL; //Signature should not require a return...
}

static uint32_t entity_star_box_new(uint32_t target)
{
	uint32_t starbox = entity_new();
	entity_add(starbox, (Target){.target = target});
	entity_add(starbox, (Label){.name = "Star Box", .description = "Stars renderered into a 3D sliding window around the camera."});
	entity_add(starbox, (ScriptableTemp){.script = entity_star_box_scriptable});
	entity_add(starbox, (CustomDrawable){.draw = entity_star_box_draw, .construct = entity_star_box_construct, .destruct = entity_star_box_destruct});
	return starbox;
}

//Position is irrelevant at the scale of a galaxy
static uint32_t entity_galaxy_new(mat3 orientation, bpos_origin origin)
{
	uint32_t galaxy = entity_new();
	entity_add(galaxy, (Label){.name = "Galaxy"});
	// entity_add(galaxy, (Physical){});
	// entity_add(galaxy, (Universal){});
	// entity_add(galaxy, (GridExplorable){});
	// entity_add(galaxy, (DistanceHysteresisExplorable){});
	// entity_add(galaxy, (ParentOf27){});
	// entity_add(galaxy, (LOD){});
	return galaxy;
}

static uint32_t entity_universe_new()
{
	uint32_t universe = entity_new();
	// entity_add_component(universe, (Label){.name = "Universe", .description = NULL})
	entity_add(universe, (PhysicalTemp){.position = (amat4)AMAT4_IDENT});
	entity_add(universe, (Label){.name = "Universe"});
	// entity_add(universe, (ScriptableTemp){.script = })
	// entity_add(universe, (Universal){});
	// entity_add(universe, (GridExplorable){});
	// entity_add(universe, (ParentOf27){});
	// entity_add(universe, (LOD){});
	//Handle big stars somehow
	//Handle dust clouds, asteroids, nebulas
	//LOD component creates/destroys
	return universe;
}

static uint32_t entity_default_camera_new(float aspect)
{
	uint32_t camera = entity_new();
	entity_add(camera, (PhysicalTemp){.position = (amat4)AMAT4_IDENT});
	entity_add(camera, (Camera){.fov = M_PI/3.0, .aspect = aspect, .near = -0.5, .far = -10000000});
	entity_add(camera, (Label){.name = "Default Camera", .description = "The primary camera used for rendering the scene."});

	return camera;
}

int universe_scene_init()
{
	float width = 800, height = 600;
	//Old-style shader loading, for star box
	load_effects(
		effects.all,       LENGTH(effects.all),
		shader_file_paths, LENGTH(shader_file_paths),
		attribute_strings, LENGTH(attribute_strings),
		uniform_strings,   LENGTH(uniform_strings));
	checkErrors("load_effects");

	glPointSize(5);
	glEnable(GL_PROGRAM_POINT_SIZE);
	checkErrors("glPointSize");

	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0.01f, 0.02f, 0.03f, 1.0f);

	e = ecs_new(10000, 50);

	//TODO(Gavin): Validate ctypes, in case a component type is uninitialized.
	for (int i = 0; i < LENGTH(component_init_params); i++) {
		ecs_component_init(E, &component_init_params[i]);
	}

	//TODO(Gavin): Copy other needed scene init stuff from space_scene (and convert to ECS stuff where reasonable)

	//Universe entity will spawn and manage galaxy entities,
	uint32_t universe = entity_universe_new();
	//which spawn and manage star entities (and other stellar phenomenon like nebulas, rogue planets, space rocks), which spawn and manage planet entities
	//Then I need a spaceship/player entity, and a camera entity

	uint32_t player = entity_new();
	default_camera = entity_default_camera_new(width/height);
	//Attach default framebuffer (fbo 0) directly to the default camera entity.
	entity_add(default_camera, (Framebuffer){.width = width, .height = height, .camera = default_camera, .ogl_fbo = 0, .render_every_frame = true});
	uint32_t starbox = entity_star_box_new(default_camera);
	checkErrors("Universe %d", __LINE__);

	return 0;
}

void universe_scene_deinit()
{
	//This currently leaks because component destructors (Ex. entity_remove_customdrawable) are not called.
	//ECS will be trimmed down to not include constructors/destructors + ctype init/deinit.
	//Destruction will be more C-style per-scene, maybe leveraging some kind of iterator functionality provided by the ECS.

	//TODO(Gavin): Maybe make a function for this kind of thing (if I end up needing to free a lot of stuff like this).
	size_t num_customdrawables = 0;
	CustomDrawable *customdrawables = ecs_components(E, ecs_ctypes.customdrawable, &num_customdrawables);
	const uint32_t *customdrawable_itoh = ecs_component_itoh(E, ecs_ctypes.customdrawable);
	for (int i = 0; i < num_customdrawables; i++)
		entity_remove_customdrawable(customdrawable_itoh[i]);

	ecs_free(E);
	checkErrors("Universe %d", __LINE__);
}

void universe_scene_update(float dt)
{
	size_t num_scriptables = 0;
	ScriptableTemp *s = ecs_components(E, ecs_ctypes.scriptable, &num_scriptables);
	//TODO: Figure out a nice way for components that need their own handle to access it.
	//Maybe ecs_components returns num, has void ** argument for component list, and const uint32_t * argument for handle list?
	struct hmempool *scriptables = &e.components[ecs_ctypes.scriptable];
	for (int i = 0; i < num_scriptables; i++) {
		s[i].script(scriptables->itoh[i]);
	}
}

void framebuffers_update()
{
	//TODO(Gavin): Framebuffer rendering should be associated with a particular set of rendered entities, not *ALL* entities.
	size_t num_framebuffers = 0, num_customdrawables = 0;
	Framebuffer *framebuffers = ecs_components(E, ecs_ctypes.framebuffer, &num_framebuffers);
	CustomDrawable *customdrawables = ecs_components(E, ecs_ctypes.customdrawable, &num_customdrawables);
	for (int i = 0; i < num_framebuffers; i++) {
		Framebuffer *fb = &framebuffers[i];
		if (fb->render_every_frame) {
			for (int j = 0; j < num_customdrawables; j++) {
				//TODO(Gavin): Bind framebuffer
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
				customdrawables[j].draw(&e, fb->camera, ecs_component_itoh(E, ecs_ctypes.customdrawable)[j], NULL);
			}
		}
	}
}

void universe_scene_render()
{
	framebuffers_update();
}

void universe_scene_resize(float width, float height)
{
	Framebuffer *fb = entity_framebuffer(default_camera);
	fb->width = width; fb->height = height;
	//Update the default camera aspect ratio and re-run the constructor to regenerate the projection matrix.
	Camera *c = entity_camera(default_camera);
	c->aspect = width/height;
	camera_constructor(default_camera, c, e.userdatas[ecs_ctypes.framebuffer]);
	glViewport(0, 0, width, height);
}
