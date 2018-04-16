#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "glla.h"
#include "space_scene.h"
#include "../scene.h"
#include "../models/models.h"
#include "../input_event.h"
#include "../default_settings.h"
#include "../buffer_group.h"
//#include "../experiments/deferred_framebuffer.h"
#include "../lights.h"
#include "../macros.h"
#include "../shader_utils.h"
#include "../math/utility.h"
#include "../space/stars.h"
#include "../space/star_box.h"
#include "../draw.h"
#include "../drawf.h"
#include "../entity/entity_components.h"
#include "../space/procedural_planet.h"
#include "../space/triangular_terrain_tile.h"
#include "../math/bpos.h"
#include "../debug_graphics.h"
#include "../space/solar_system.h"
#include "../glsw_shaders.h"

SCENE_VTABLE(space);
float FOV = M_PI/2.7;
float far_distance = 10000000;
float near_distance = 1;
float log_depth_intermediate_factor = NAN; //Needs to be set in init.
int PRIMITIVE_RESTART_INDEX = 0xFFFFFFFF;

float screen_width, screen_height;

GLuint gVAO = 0;
amat4 inv_eye_frame;
static amat4 eye_frame = {.a = MAT3_IDENT,  .t = {6, 0, 0}};
bpos_origin eye_sector = {0, 0, 0};
//Object frames. Need a better system for this.
amat4 room_frame = {.a = MAT3_IDENT, .t = {0, -4, -8}};
amat4 grid_frame = {.a = MAT3_IDENT, .t = {-50, -90, -550}};
amat4 tri_frame  = {.a = MAT3_IDENT, .t = {0, 0, 0}};
bpos_origin room_sector = {0, 0, 0};
amat4 big_asteroid_frame = {.a = MAT3_IDENT, .t = {0, -4, -20}};
amat4 skybox_frame; //Set in init_render() and update()
float skybox_scale;
vec3 ambient_color = {0.01, 0.01, 0.01};
vec3 sun_direction = {0.1, 0.8, 0.1};
vec3 sun_color     = {0.1, 0.8, 0.1};
struct point_light_attributes point_lights = {.num_lights = 0};


const float planet_radius = 6000000;

solar_system ssystem;

Entity *ship_entity = NULL;
Entity *camera_entity = NULL;
Entity *star_box_entity;

GLfloat proj_mat[16];
GLfloat proj_view_mat[16];

Drawable d_ship, d_newship, d_teardropship, d_room, d_skybox;
Entity *entities;
int16_t nentities;

/*
//Just a hacky record so that I allocate/free all these properly as I develop this.
struct drawable_rec {
	Drawable *drawable;
	Draw_func draw;
	EFFECT *effect;
	amat4 *frame;
	bpos_origin *sector;
	int (*buffering_function)(struct buffer_group);
} drawables[] = {
	{&d_ship,         draw_forward,        &effects.forward, &ship->Physical->position, &ship->Physical->origin,   buffer_teardropship},
	{&d_newship,      draw_forward,        &effects.forward, &ship->Physical->position, &ship->Physical->origin,   buffer_newship     },
	{&d_teardropship, draw_forward,        &effects.forward, &ship->Physical->position, &ship->Physical->origin,   buffer_teardropship},
	{&d_room,         draw_forward,        &effects.forward, &room_frame,    &room_sector,   buffer_newroom     },
	{&d_skybox,       draw_skybox_forward, &effects.skybox,  &skybox_frame,  &eye_sector,    buffer_cube        }
};
*/


static void init_models()
{
	//In the future, this function should be called in a loop on all entities with the drawable component.
	//Maybe configured from some config file?
	//init_heap_drawable(Drawable *drawable, Draw_func draw, EFFECT *effect, amat4 *frame, int (*buffering_function)(struct buffer_group));
	// for (int i = 0; i < LENGTH(drawables); i++)
	// 	init_heap_drawable(drawables[i].drawable, drawables[i].draw, drawables[i].effect, drawables[i].frame, drawables[i].sector, drawables[i].buffering_function);

	//test_planets[0].planet = proc_planet_new(planet_radius, proc_planet_height, test_planets[0].col);
	//test_planets[1].planet = proc_planet_new(planet_radius * 1.7, proc_planet_height, test_planets[1].col);
	ssystem = solar_system_new(eye_sector);

	//bpos_split_fix(&ship.position.t, &ship.sector);
}

static void deinit_models()
{
	// for (int i = 0; i < LENGTH(drawables); i++)
	// 	deinit_drawable(drawables[i].drawable);
	solar_system_free(ssystem);

	//for (int i = 0; i < LENGTH(test_planets); i++)
		//proc_planet_free(test_planets[i].planet);
}

void entities_init()
{
	ship_entity = entity_new(PHYSICAL_BIT | CONTROLLABLE_BIT | DRAWABLE_BIT);
	ship_entity->Physical->position.t = (vec3){0, 6000 + planet_radius, 0};
	ship_entity->Physical->origin = (bpos_origin){0, 400, 0};
	ship_entity->Controllable->control = ship_control;
	// printf("ship_entity: %p\n", ship_entity);
	// printf("ship_entity->Controllable: %p\n", ship_entity->Controllable);
	// printf("ship_entity->Controllable->context: %p\n", ship_entity->Controllable->context);
	//TODO: Init drawable part

	camera_entity = entity_new(PHYSICAL_BIT | CONTROLLABLE_BIT | SCRIPTABLE_BIT);
	camera_entity->Physical->position.t = (vec3){0, 4, 8};
	camera_entity->Controllable->control = camera_control;
	camera_entity->Controllable->context = ship_entity;
	// printf("camera_entity: %p\n", camera_entity);
	// printf("camera_entity->Controllable: %p\n", camera_entity->Controllable);
	// printf("camera_entity->Controllable->context: %p\n", camera_entity->Controllable->context);

	camera_entity->Scriptable->script = camera_script;
	camera_entity->Scriptable->context = ship_entity;

	star_box_entity = entity_new(SCRIPTABLE_BIT);
	star_box_entity->Scriptable->script = star_box_script;
	star_box_entity->Scriptable->context = camera_entity;
}

void entities_deinit()
{
	entity_reset();
}

static void init_lights()
{
	//                             Position,            color,                atten_c, atten_l, atten_e, intensity
	new_point_light(&point_lights, (vec3){0, 2, 0},   (vec3){1.0, 0.2, 0.2}, 0.0,     0.0,    1,    200);
	new_point_light(&point_lights, (vec3){0, 2, 0},   (vec3){1.0, 1.0, 1.0}, 0.0,     0.0,    1,    5);
	new_point_light(&point_lights, (vec3){0, 2, 0},   (vec3){0.8, 0.8, 1.0}, 0.0,     0.3,    0.04, 0.4);
	new_point_light(&point_lights, (vec3){10, 4, -7}, (vec3){0.4, 1.0, 0.4}, 0.1,     0.14,   0.07, 0.75);
	point_lights.shadowing[0] = true;
	point_lights.shadowing[1] = true;
	//point_lights.shadowing[2] = true;
	//point_lights.shadowing[3] = true;
}

void space_scene_resize(float width, float height)
{
	glViewport(0, 0, width, height);
	screen_width = width;
	screen_height = height;
	make_projection_matrix(FOV, screen_width/screen_height, -near_distance, -far_distance, proj_mat);	
}

//Set up everything needed to start rendering frames.
int space_scene_init()
{
	checkErrors("Function enter");

	glsw_shaders_init();
	checkErrors("glsw_shaders_init");

	load_effects(
		effects.all,       LENGTH(effects.all),
		shader_file_paths, LENGTH(shader_file_paths),
		attribute_strings, LENGTH(attribute_strings),
		uniform_strings,   LENGTH(uniform_strings));
	checkErrors("load_effects");

	glGenVertexArrays(1, &gVAO);

	glUseProgram(0);
	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0.01f, 0.02f, 0.03f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	checkErrors("glClear");
	log_depth_intermediate_factor = 2.0/log2(far_distance + 1.0);

	entities_init();
	checkErrors("Entities");
	init_models();
	checkErrors("Models");
	init_lights();
	checkErrors("Lights");
	//stars_init();
	checkErrors("Init stars");
	star_box_init(eye_sector);
	checkErrors("Init star_box");
	debug_graphics_init();
	checkErrors("Init debug_graphics");
	proc_planet_init();


	glUseProgram(effects.forward.handle);
	glUniform1f(effects.forward.log_depth_intermediate_factor, log_depth_intermediate_factor);

	checkErrors("forward log_depth_intermediate_factor");

	glUseProgram(effects.skybox.handle);
	glUniform1f(effects.skybox.log_depth_intermediate_factor, log_depth_intermediate_factor);

	checkErrors("skybox log_depth_intermediate_factor");

	skybox_scale = 2*far_distance / sqrt(3);
	skybox_frame.a = mat3_scalemat(skybox_scale, skybox_scale, skybox_scale);
	skybox_frame.t = eye_frame.t;

	glPointSize(5);
	glEnable(GL_PROGRAM_POINT_SIZE);
	checkErrors("glPointSize");

	entity_update_components();

	glUseProgram(0);
	checkErrors("After init_render");

	return 0;
}

void space_scene_deinit()
{
	deinit_models();
	//stars_deinit();
	proc_planet_deinit();
	point_lights.num_lights = 0;
	entities_deinit();
}

static void forward_update_point_light(EFFECT *effect, struct point_light_attributes *lights, int i)
{
	vec3 position = lights->position[i];
	glUniform3f(effect->uLight_pos, position.x, position.y, position.z);
	glUniform3f(effect->uLight_col, position.x, position.y, position.z);
	glUniform4f(effect->uLight_attr, lights->atten_c[i], lights->atten_l[i], lights->atten_e[i], lights->intensity[i]);
}

//Eventually I'll have an algorithm to calculate potential visible set, etc.
//static Drawable *pvs[] = {&d_newship, &d_room};
static Drawable *pvs[] = {};//{&d_ship};

void space_scene_render()
{
	// //Create a list of planet tiles to draw.
	// int drawlist_max = 3000;
	// tri_tile *drawlist[ssystem.num_planets][drawlist_max];
	// int drawlist_count[ssystem.num_planets];
	// for (int i = 0; i < ssystem.num_planets; i++) {
	// 	bpos pos = {eye_frame.t - ssystem.planet_positions[i].offset, eye_sector - ssystem.planet_positions[i].origin};
	// 	drawlist_count[i] = proc_planet_drawlist(ssystem.planets[i], drawlist[i], drawlist_max, pos);
	// }

	//Future Gavin Reminder: You put this here so it can set the intersecting tile's override color before the draw.
	//bpos ray_start = {ship_entity->Physical->position.t - test_planets[0].pos.offset, ship_entity->Physical->origin - test_planets[0].pos.origin};
	//bpos intersection = {{0}};
	//float ship_altitude = proc_planet_altitude(test_planets[0].planet, ray_start, &intersection);
	//printf("Current ship altitude to planet 0: %f\n", ship_altitude);

	//float h = vec3_dist(eye_frame.t, test_planet->pos) - test_planet->radius; //If negative, we're below sea level.
	//printf("Height: %f\n", h);
	static float hella_time = 0.0;
	hella_time += 1.0/60.0;
	glUseProgram(effects.stars.handle);
	glUniform1f(effects.stars.hella_time, hella_time);
	glUseProgram(effects.forward.handle);
	glUniform1f(effects.forward.hella_time, hella_time);

	bool wireframe = false;

	//Compute inverse eye frame.
	{
		inv_eye_frame = amat4_inverse(eye_frame);
		float tmp[16];
		amat4_to_array(inv_eye_frame, tmp);
		amat4_buf_mult(proj_mat, tmp, proj_view_mat);
	}

	glDepthMask(GL_TRUE);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);

	//If we're outside the shadow volume, we can use z-pass instead of z-fail.
	//z-pass is faster, and not patent-encumbered.
	int zpass = key_state[SDL_SCANCODE_7];
	int apass = !key_state[SDL_SCANCODE_5]; //Ambient pass

	//Draw in wireframe if 'z' is held down.
	wireframe = key_state[SDL_SCANCODE_Z];
	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//Depth prepass, can also be used as an ambient pass.
	{
		glUseProgram(effects.forward.handle);
		if (apass) {
			glUniform1i(effects.forward.ambient_pass, 1);
			glUniform3f(effects.forward.uLight_col, VEC3_COORDS(sun_color));
			vec3 sun = bpos_remap((bpos){{0,0,0}, ssystem.origin}, eye_sector);
			glUniform3f(effects.forward.uLight_pos, VEC3_COORDS(sun)); //was sun_direction
		} else {
			glDrawBuffer(GL_NONE); //Disable drawing to the color buffer if no ambient pass, save on fillrate.
		}
		glUniform3f(effects.forward.camera_position, VEC3_COORDS(eye_frame.t));

		//Draw entities
		for (int i = 0; i < LENGTH(pvs); i++)
			draw_drawable(pvs[i]);

		//glDisable(GL_CULL_FACE);

		checkErrors("Before planets draw");
		proc_planet_draw(eye_frame, proj_view_mat, ssystem.planets, ssystem.planet_positions, ssystem.num_planets);
		glUseProgram(effects.forward.handle);

		glUniform3f(effects.forward.override_col, 1.0, 1.0, 1.0);

		checkErrors("After drawing into depth");
		glUniform1i(effects.forward.ambient_pass, 0);
	}

	//For each light, draw potential occluders to set the stencil buffer.
	//Then draw the scene, accumulating non-occluded light onto the models additively.
	for (int i = 0; i < point_lights.num_lights; i++) {
		//Render shadow volumes into the stencil buffer.
		if (point_lights.shadowing[i]) {
			glEnable(GL_DEPTH_CLAMP);
			glDepthFunc(GL_LESS);
			glDisable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_ALWAYS, 0, 0xff);

			if (zpass) {
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP); 
			} else {
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
			}

			glUseProgram(effects.shadow.handle);
			glUniform3f(effects.shadow.gLightPos, VEC3_COORDS(point_lights.position[i]));
			glUniform1i(effects.shadow.zpass, zpass);

			for (int i = 0; i < LENGTH(pvs); i++)
				draw_forward_adjacent(&effects.shadow, *pvs[i]->bg, *pvs[i]->frame);

			glDisable(GL_DEPTH_CLAMP);
			glEnable(GL_CULL_FACE);
			checkErrors("After rendering shadow volumes");
		}
		//Draw the shadowed scene.
		if (point_lights.enabled_for_draw[i]) {
			glDrawBuffer(GL_BACK);
			glStencilFunc(GL_EQUAL, 0x0, 0xFF);
			glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
			glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP);
			glUseProgram(effects.forward.handle);
			forward_update_point_light(&effects.forward, &point_lights, i);
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_ONE, GL_ONE);
			glDepthFunc(GL_EQUAL);

			checkErrors("Before forward draw shadowed");
			for (int i = 0; i < LENGTH(pvs); i++) {
				draw_drawable(pvs[i]);
				checkErrors("After forward draw shadowed");
			}

			// //Draw procedural planet
			// for (DRAWLIST l = terrain_list; l; l = l->next) {
			// 	draw_forward(&effects.forward, l->t->bg, tri_frame);
			// 	checkErrors("After drawing a tri_tile");
			// }

			glDisable(GL_BLEND);
			checkErrors("After drawing shadowed");
		}
		glClear(GL_STENCIL_BUFFER_BIT);
	}
	glDisable(GL_BLEND);
	glDepthFunc(GL_LESS);

	glUseProgram(effects.skybox.handle);
	glUniform3f(effects.skybox.sun_direction, VEC3_COORDS(sun_direction));
	glUniform3f(effects.skybox.sun_color, VEC3_COORDS(sun_color));
	skybox_frame.t = eye_frame.t;
	//draw_drawable(&d_skybox);
	//stars_draw(eye_frame, proj_view_mat);
	star_box_draw(eye_sector, proj_view_mat);

	debug_graphics.lines.ship_to_planet.start = bpos_remap((bpos){ship_entity->Physical->position.t, ship_entity->Physical->origin}, eye_sector);
	debug_graphics.lines.ship_to_planet.end   = bpos_remap(ssystem.planet_positions[0], eye_sector);
	debug_graphics.lines.ship_to_planet.enabled = true;
	//debug_graphics.points.ship_to_planet_intersection.pos = bpos_remap(intersection, eye_sector);
	//debug_graphics.points.ship_to_planet_intersection.enabled = true;
	//debug_graphics_draw(eye_frame, proj_view_mat);

	checkErrors("After forward junk");
}

//TODO: Move the update function out of the renderer module, come up with a good interface for things that need to be accessed in both.
void space_scene_update(float dt)
{
	static float light_time = 0;
	light_time += dt * 0.2;
	point_lights.position[0] = (vec3){10*cos(light_time), 4, 10*sin(light_time)-8};

	//Commented out because I kept hitting y and getting annoyed.
	// if (key_state[SDL_SCANCODE_Y]) {
	// 	printf("Skybox scale is %f, what would you like the scale to be?\n", skybox_scale);
	// 	scanf("%f", &skybox_scale);
	// 	skybox_frame.a = mat3_scalemat(skybox_scale, skybox_scale, skybox_scale);
	// }

	entity_update_components();

	//bpos ray_start = {ship_entity->Physical->position.t - test_planets[0].pos.offset, ship_entity->Physical->origin - test_planets[0].pos.origin};
	//bpos intersection = {0};
	//float ship_altitude = proc_planet_altitude(test_planets[0].planet, ray_start, &intersection);
	//printf("Current ship altitude to planet 0: %f\n", ship_altitude);

	eye_frame = (amat4){camera_entity->Physical->position.a, amat4_multpoint(ship_entity->Physical->position, camera_entity->Physical->position.t)};
	eye_sector = ship_entity->Physical->origin;
	bpos_split_fix(&eye_frame.t, &eye_sector);

	point_lights.enabled_for_draw[2] = key_state[SDL_SCANCODE_6];
	
	static float sunscale = 50.0;
	if (key_state[SDL_SCANCODE_EQUALS])
		sunscale += 0.1;
	if (key_state[SDL_SCANCODE_MINUS])
		sunscale -= 0.1;
	sun_color = ambient_color * sunscale;
}
