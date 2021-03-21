#include "universe_scene.h"
#include "glla.h"
#include "math/bpos.h"
#include "math/utility.h"
#include "macros.h"
#include "shader_utils.h"
#include "trackball/trackball.h"
#include "space/star_box.h"
#include "input_event.h"
#include "experiments/universe_scene/universe_ecs.h"
#include "experiments/universe_scene/universe_components.h"
#include "experiments/universe_scene/universe_entities/gpu_planet.h"
#include <math.h>
#include <assert.h>
SCENE_IMPLEMENT(universe)

struct game_scene universe_scene;
uint32_t default_camera = 0;
uint32_t test_planet = 0;
struct entity_ctypes universe_ecs_ctypes = {0};
static ecs_ctx universe_ecs_ctx = {.user_ctypes = &universe_ecs_ctypes};
ecs_ctx *puniverse_ecs_ctx = &universe_ecs_ctx;
//Short names for convenience
#define E puniverse_ecs_ctx
#define e universe_ecs_ctx
#define ctypes universe_ecs_ctypes

//This doesn't need to be in the ECS because its state can be reconstructed from the camera constructor.
static struct trackball camera_trackball;

//Set up all the components needed for this scene
static struct ecs_component_init_params component_init_params[] = {
//    num, size, constructor, destructor, userdata, ctype out, ECS out, init params out, init, deinit
	{.num = 10,  .size = sizeof(PhysicalTemp),   .ctype = &ctypes.physical},
	{.num = 10,  .size = sizeof(Camera),         .ctype = &ctypes.camera,         .construct = camera_constructor},
	{.num = 500, .size = sizeof(CustomDrawable), .ctype = &ctypes.customdrawable, .construct = customdrawable_constructor, .destruct = customdrawable_destructor},
	{.num = 20,  .size = sizeof(Label),          .ctype = &ctypes.label,          .construct = label_constructor,          .destruct = label_destructor},
	{.num = 500, .size = sizeof(ScriptableTemp), .ctype = &ctypes.scriptable},
	{.num = 10,  .size = sizeof(Target),         .ctype = &ctypes.target},
	{.num = 5,   .size = sizeof(Trackball),      .ctype = &ctypes.trackball,      .construct = trackball_constructor},
	// {10, sizeof(Universal), NULL, NULL, NULL, NULL, NULL, component_universal_init, 0, NULL},}
};

//Gavin 2020/05/22: Why did I have this? Shouldn't I only use the target origin?
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

scriptabletemp_callback(entity_star_box_script)
{
	bpos_origin origin = {0,0,0};
	target_or_self_origin(eid, &origin);
	star_box_update(entity_customdrawable(eid)->ctx, origin);
}

customdrawable_callback(entity_star_box_draw)
{
	Camera *c = entity_camera(camera);
	assert(c);
	glUseProgram(effects.star_box.handle);
	glUniform1f(effects.star_box.log_depth_intermediate_factor, c->log_depth_intermediate_factor);
	star_box_draw(entity_customdrawable(self)->ctx, entity_physicaltemp(camera)->origin, c->proj_view_mat);
	checkErrors("Universe %d", __LINE__);
}

CD(entity_star_box_construct)
{
	bpos_origin origin = {0,0,0};
	target_or_self_origin(eid, &origin);
	((CustomDrawable *)component)->ctx = star_box_init(star_box_new(), origin);
}

CD(entity_star_box_destruct)
{
	star_box_deinit(((CustomDrawable *)component)->ctx);
}

static uint32_t entity_star_box_new(uint32_t target)
{
	uint32_t starbox = entity_new();
	entity_add(starbox, (Target){.target = target});
	entity_add(starbox, (Label){.name = "Star Box", .description = "Stars renderered into a 3D sliding window around the camera."});
	entity_add(starbox, (ScriptableTemp){.script = entity_star_box_script});
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
	entity_add(universe, (PhysicalTemp){.position = (amat4)AMAT4_IDENT, .velocity = (amat4)AMAT4_IDENT, .acceleration = (amat4)AMAT4_IDENT});
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

scriptabletemp_callback(entity_camera_script)
{
	PhysicalTemp *c = entity_physicaltemp(eid);
	PhysicalTemp *p = entity_physicaltemp(entity_target(eid)->target);
	Trackball *t = entity_trackball(eid);
	assert(c && p && t);

	c->origin = p->origin;
	int mouse_x = 0, mouse_y = 0;
	Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
	bool button = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
	int scroll_x = input_mouse_wheel_sum.wheel.x, scroll_y = input_mouse_wheel_sum.wheel.y;
	trackball_step(&t->trackball, mouse_x, mouse_y, button, scroll_x, scroll_y);

	c->position = t->trackball.camera;
	// qvec3_println(c->origin);
	// c->position.a = mat3_lookat(
	// 	mat3_multvec(p->position.a, c->position.t) + mat3_multvec(p->position.a, p->position.t), //Look source, relative to p.
	// 	mat3_multvec(p->position.a, p->position.t), //Look target (p), relative to p.
	// 	mat3_multvec(p->position.a, (vec3){0, 1, 0})); //Up vector, relative to p.
}

static uint32_t entity_default_camera_new(uint32_t target, float width, float height)
{
	uint32_t camera = entity_new();
	entity_add(camera, (PhysicalTemp){.position = (amat4)AMAT4_IDENT, .velocity = (amat4)AMAT4_IDENT, .acceleration = (amat4)AMAT4_IDENT});
	entity_add(camera, (ScriptableTemp){.script = entity_camera_script});
	entity_add(camera, (Camera){.fov = M_PI/3.0, .aspect = width/height, .near = -0.5, .far = -10000000, .width = width, .height = height});
	entity_add(camera, (Target){.target = target});
	entity_add(camera, (Trackball){.target = target});
	entity_add(camera, (Label){.name = "Default Camera", .description = "The primary camera used for rendering the scene."});

	return camera;
}

scriptabletemp_callback(entity_player_script)
{
	PhysicalTemp *p = entity_physicaltemp(eid);
	assert(p);
	if (key_state[SDL_SCANCODE_W])
		p->velocity.t.z -= 10000000000.0;
	if (key_state[SDL_SCANCODE_S])
		p->velocity.t.z += 10000000000.0;

	printf("Printing player stuff:\n");
	qvec3_println(p->origin);
	vec3_println(p->position.t);

	PhysicalTemp *pp = entity_physicaltemp(test_planet);
	gpu_planet *gp = entity_scriptable(test_planet)->context;
	bpos intersection;
	printf("Printing planet stuff:\n");
	qvec3_println(pp->origin);
	vec3_println(pp->position.t);
	printf("Altitude: %f\n", gpu_planet_altitude(gp, (bpos){p->position.t, p->origin}, &intersection));
}

static uint32_t entity_player_new()
{
	uint32_t player = entity_new();
	entity_add(player, (ScriptableTemp){.script = entity_player_script});
	entity_add(player, (PhysicalTemp){.position = {.a = MAT3_IDENT, .t = {0,0,-10}}});
	return player;
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

	gpu_planet_init();
	checkErrors("gpu_planet_init");

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

	uint32_t player = entity_player_new();
	default_camera = entity_default_camera_new(player, width, height);
	uint32_t starbox = entity_star_box_new(default_camera);

	uint32_t planet = entity_gpu_planet_new();
	test_planet = planet;
	checkErrors("Universe %d", __LINE__);

	return 0;
}

void universe_scene_deinit()
{
	gpu_planet_deinit();
	ecs_free(E);
	checkErrors("Universe %d", __LINE__);
}

//TODO: Move this to its own file when I make a proper Physical "system"
static void component_physical_update(struct component_physicaltemp *p)
{
	p->velocity.t += p->acceleration.t;
	p->position.t += p->velocity.t;
	p->velocity.a = mat3_mult(p->velocity.a, p->acceleration.a);
	p->position.a = mat3_mult(p->position.a, p->velocity.a);

	bpos_split_fix(&p->position.t, &p->origin);
}

void universe_print()
{
	size_t num_labels = 0;
	Label *labels = ecs_components(E, ctypes.label, &num_labels);
	const uint32_t *labels_itoh = ecs_component_itoh(E, ctypes.label);
	for (int i = 0; i < num_labels; i++) {
		printf("[Entity %u][%s] %s\n", labels_itoh[i], labels[i].name ? labels[i].name : "", labels[i].description ? labels[i].description : "");
	}
}

void universe_scene_update(float dt)
{
	size_t num_scriptables = 0, num_physicals = 0;
	ScriptableTemp *scriptables = ecs_components(E, ctypes.scriptable, &num_scriptables);
	const uint32_t *scriptables_itoh = ecs_component_itoh(E, ctypes.scriptable);
	for (int i = 0; i < num_scriptables; i++)
		scriptables[i].script(scriptables_itoh[i]);

	PhysicalTemp *p = ecs_components(E, ctypes.physical, &num_physicals);
	for (int i = 0; i < num_physicals; i++)
		component_physical_update(&p[i]);
}

void camera_recursive_add(struct mempool *m, uint32_t camera)
{
	Camera *c = entity_camera(camera);
	if (c->num_prev == 0 && !c->visited) {
		c->visited = true;
		mempool_add(m, &camera);
		if (c->next) {
			entity_camera(c->next)->num_prev--;
			camera_recursive_add(m, c->next);
		}
	}
}

void universe_scene_render()
{
	/*
	For now, I'm only concerned about the main camera and the default framebuffer.
	Later I will modify this to consider multiple cameras with their own attached framebuffers.
	This can be used for screens, reflection buffers, the galaxy skybox, etc.
	Basically all render-to-texture stuff.

	This function is currently working as the renderer "system"
	*/
	if (key_pressed(SDL_SCANCODE_P)) {
		universe_print();
	}

	PhysicalTemp *camera_physical = entity_physicaltemp(default_camera);
	Camera *camera_camera = entity_camera(default_camera);
	amat4 inv_eye_frame = amat4_inverse(camera_physical->position);
	float tmp[16];
	amat4_to_array(inv_eye_frame, tmp);
	amat4_buf_mult(camera_camera->proj_mat, tmp, camera_camera->proj_view_mat);

	size_t num_cameras = 0;
	Camera *cameras = ecs_components(E, ctypes.camera, &num_cameras);
	struct mempool cameras_sorted = mempool_new(num_cameras, sizeof(uint32_t));

	//Set all cameras to unvisited, find the number of precursors for each.
	//num_prev assumed to be 0-init, or reset to 0 after previous frame's render.
	for (int i = 0; i < num_cameras; i++) {
		cameras[i].visited = false;
		if (cameras[i].next)
			entity_camera(cameras[i].next)->num_prev++;
	}

	//Add each camera in order to topologically sort
	const uint32_t *cameras_itoh = ecs_component_itoh(E, ctypes.camera);
	for (int i = 0; i < num_cameras; i++)
		camera_recursive_add(&cameras_sorted, cameras_itoh[i]);

	//Render each camera
	for (int i = 0; i < cameras_sorted.num; i++) {
		uint32_t camera = *(uint32_t *)mempool_get(&cameras_sorted, i);
		Camera *c = entity_camera(camera);
		//This is where I would set the framebuffer target

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		size_t num_customdrawables = 0;
		CustomDrawable *customdrawables = ecs_components(E, ctypes.customdrawable, &num_customdrawables);
		for (int j = 0; j < num_customdrawables; j++)
			if ((customdrawables[j].draw_group & c->draw_groups) == c->draw_groups)
				customdrawables[j].draw(&e, camera, ecs_component_itoh(E, ctypes.customdrawable)[j], NULL);

		//This is where I will draw all the "normal" Drawables (after the CustomDrawables)
	}

	mempool_delete(&cameras_sorted);
}

void universe_scene_resize(float width, float height)
{
	//Update the default camera aspect ratio and re-run the constructor to regenerate the projection matrix.
	Camera *c = entity_camera(default_camera);
	c->width = width; c->height = height;
	c->aspect = width/height;
	camera_constructor(default_camera, c, NULL);
	glViewport(0, 0, width, height);
}

/*
TODO 2020/05/20

- create a mesh system so I can draw the spaceship
	can adapt existing Lua script for now, switch to C for performance later
- add a controller to the player so it can be controlled
- decouple the "spiral" shader from its scene, create geometry that covers the same screen area to drive raymarching
- create "uniform" component that retrieves uniform name from a shader and updates a shader's uniform values
	-Or not, I could just drive this from a script...
- create a UI slider component that can render and modify a value on a particular entity's component at some offset
- create a UI system that can organize and render UI components (also toggle everything on and off)
	something like add_ui_parent to create a tree of UI components?
- add another layer of "guide stars" that are visible from a distance
- spawn real stars when player is close to them (can use qhypertoroidal_buffer_slot again, with a smaller range)
- create a "free_tree" component that will destroy entities when their lifetime parent entity has been removed (only when a collection pass is run)
	useful for small number of infrequently-changing things (UI elements, maybe planets?)
- decide if physical components should have a constructor to give sensible default values to the acc/vel/pos matrices
	will this be necessary if I switch to quarternions or some other representation?

TODO 2020/09/13
- BUG: figure out why sometimes when I launch this scene, it's all black
- bring in all the draw pass stuff from space scene (shadows included)
*/

