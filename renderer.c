#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <GL/glew.h>
#include "glla.h"
#include "renderer.h"
#include "models/models.h"
#include "input_event.h"
#include "default_settings.h"
#include "buffer_group.h"
#include "deferred_framebuffer.h"
#include "lights.h"
#include "macros.h"
#include "shader_utils.h"
#include "gl_utils.h"
#include "stars.h"
#include "draw.h"
#include "drawf.h"
#include "drawable.h"
#include "procedural_planet.h"
#include "triangular_terrain_tile.h"
#include "ship_control.h"
#include "math/space_sector.h"

static bool renderer_is_init = false;
static bool renderer_should_reload = false;
float FOV = M_PI/3.0;
float far_distance = 1000000000;
float near_distance = 1;
int PRIMITIVE_RESTART_INDEX = 0xFFFFFFFF;

float screen_width = SCREEN_WIDTH;
float screen_height = SCREEN_HEIGHT;

GLuint gVAO = 0;
amat4 inv_eye_frame;
amat4 eye_frame = {.a = MAT3_IDENT,          .t = {6, 0, 0}};
space_sector eye_sector = {0, 0, 0};
//Object frames. Need a better system for this.
amat4 ship_frame = {.a = MAT3_IDENT,         .t = {-2.5, 0, -8}};
amat4 newship_frame = {.a = MAT3_IDENT,      .t = {3, 0, -8}};
amat4 teardropship_frame = {.a = MAT3_IDENT, .t = {6, 0, -8}};
amat4 room_frame = {.a = MAT3_IDENT,         .t = {0, -4, -8}};
amat4 grid_frame = {.a = MAT3_IDENT,         .t = {-50, -90, -550}};
amat4 tri_frame  = {.a = MAT3_IDENT,         .t = {0, 0, 0}};
space_sector tri_sector = {0, 0, 0};
amat4 tri_frame_eye_relative  = {.a = MAT3_IDENT, .t = {0, 0, 0}};
amat4 big_asteroid_frame = {.a = MAT3_IDENT, .t = {0, -4, -20}};
amat4 skybox_frame; //Set in init_render() and update()
vec3 ambient_color = {0.01, 0.01, 0.01};
vec3 sun_direction = {0.1, 0.8, 0.1};
vec3 sun_color     = {0.1, 0.8, 0.1};
struct point_light_attributes point_lights = {.num_lights = 0};

struct ship_physics ship = {
	.speed = 10,
	.acceleration = (vec3){0, 0, 0},
	.velocity     = AMAT4_IDENT,
	.position      = {.a = MAT3_IDENT, .t = {0, 6000500, 0}},
	.locked_camera = {.a = MAT3_IDENT, .t = {0, 4, 8}},
	.eased_camera  = {.a = MAT3_IDENT, .t = {0, 4, 8}},
	.locked_camera_target = (vec3){0, 0, -4},
	.eased_camera_target  = (vec3){0, 0, -4},
	.movable_camera       = (vec3){0, 4, 8},
	.sector               = (space_sector){0, 0, 0}
};

GLfloat proj_mat[16];
GLfloat proj_view_mat[16];

Drawable d_ship, d_newship, d_teardropship, d_room, d_skybox;

proc_planet *test_planet = NULL;
float planet_radius = 6000000;
vec3 planet_center = {0, 0, 0};;

//Just a hacky record so that I allocate/free all these properly as I develop this.
struct drawable_rec {
	Drawable *drawable;
	Draw_func draw;
	EFFECT *effect;
	amat4 *frame;
	int (*buffering_function)(struct buffer_group);
} drawables[] = {
	{&d_ship,         draw_forward,        &effects.forward, &ship.position,      buffer_ship        },
	{&d_newship,      draw_forward,        &effects.forward, &newship_frame,      buffer_newship     },
	{&d_teardropship, draw_forward,        &effects.forward, &teardropship_frame, buffer_teardropship},
	{&d_room,         draw_forward,        &effects.forward, &room_frame,         buffer_newroom     },
	{&d_skybox,       draw_skybox_forward, &effects.skybox,  &skybox_frame,       buffer_cube        }
};

static void init_models()
{
	//In the future, this function should be called in a loop on all entities with the drawable component.
	//Maybe configured from some config file?
	//init_heap_drawable(Drawable *drawable, Draw_func draw, EFFECT *effect, amat4 *frame, int (*buffering_function)(struct buffer_group));
	for (int i = 0; i < LENGTH(drawables); i++)
		init_heap_drawable(drawables[i].drawable, drawables[i].draw, drawables[i].effect, drawables[i].frame, drawables[i].buffering_function);

	test_planet = proc_planet_new(planet_center, planet_radius, tri_height_map);
}

static void deinit_models()
{
	for (int i = 0; i < LENGTH(drawables); i++)
		deinit_drawable(drawables[i].drawable);

	proc_planet_free(test_planet);
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

void handle_resize(int width, int height)
{
	glViewport(0, 0, width, height);
	screen_width = width;
	screen_height = height;
	make_projection_matrix(FOV, screen_width/screen_height, -near_distance, -far_distance, proj_mat, LENGTH(proj_mat));	
}

//Set up everything needed to start rendering frames.
void renderer_init()
{
	if (renderer_is_init)
		return;
	glUseProgram(0);
	glClearDepth(1);
	//glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0.01f, 0.02f, 0.03f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	make_projection_matrix(FOV, screen_width/screen_height, -1, -far_distance, proj_mat, LENGTH(proj_mat));

	init_models();
	init_lights();
	init_stars();

	glUseProgram(effects.forward.handle);
	glUniform1f(effects.forward.log_depth_intermediate_factor, 2.0/log(far_distance/near_distance));
	glUniform1f(effects.forward.near_plane_dist, near_distance);

	float skybox_distance = sqrt((far_distance*far_distance)/2);
	skybox_frame.a = mat3_scalemat(skybox_distance, skybox_distance, skybox_distance);
	skybox_frame.t = eye_frame.t;

	glPointSize(3);
	glEnable(GL_PROGRAM_POINT_SIZE);

	glUseProgram(0);
	checkErrors("After init_render");
	renderer_is_init = true;
}

void renderer_deinit()
{
	if (!renderer_is_init)
		return;
	deinit_models();
	deinit_stars();
	point_lights.num_lights = 0;
	renderer_is_init = false;
}

void renderer_reload()
{
	load_effects(
		effects.all,       LENGTH(effects.all),
		shader_file_paths, LENGTH(shader_file_paths),
		attribute_strings, LENGTH(attribute_strings),
		uniform_strings,   LENGTH(uniform_strings));
	renderer_deinit();
	renderer_init();
}

void renderer_queue_reload()
{
	renderer_should_reload = true;
}

//Create a projection matrix with "fov" field of view, "a" aspect ratio, n and f near and far planes.
//Stick it into buffer buf, ready to send to OpenGL.
void make_projection_matrix(GLfloat fov, GLfloat a, GLfloat n, GLfloat f, GLfloat *buf, int buf_len)
{
	assert(buf_len == 16);
	GLfloat nn = 1.0/tan(fov/2.0);
	GLfloat tmp[] = {
		nn, 0,           0,              0,
		0, nn,           0,              0,
		0,  0, (f+n)/(f-n), (-2*f*n)/(f-n),
		0,  0,          -1,              1
	};
	assert(buf_len == LENGTH(tmp));
	if (a >= 0)
		tmp[0] /= a;
	else
		tmp[5] *= a;
	memcpy(buf, tmp, sizeof(tmp));
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
static Drawable *pvs[] = {&d_ship};

void render()
{
	terrain_tree_drawlist terrain_list = NULL;
	proc_planet_drawlist(test_planet, &terrain_list, eye_frame.t);
	//float h = vec3_dist(eye_frame.t, test_planet->pos) - test_planet->radius; //If negative, we're below sea level.
	//printf("Height: %f\n", h);
	static float hella_time = 0.0;
	hella_time += 1.0/60.0;
	glUseProgram(effects.stars.handle);
	glUniform1f(effects.stars.hella_time, hella_time);
	glUseProgram(effects.forward.handle);
	glUniform1f(effects.forward.hella_time, hella_time);

	bool wireframe = false;
	//This is really the wrong place to put all this.
	{
		inv_eye_frame = amat4_inverse(eye_frame); //Only need to do this once per frame.
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
	draw_stars();

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
			glUniform3fv(effects.forward.uLight_col, 1, (float *)&sun_color);
			glUniform3fv(effects.forward.uLight_pos, 1, (float *)&sun_direction);
		} else {
			glDrawBuffer(GL_NONE); //Disable drawing to the color buffer if no ambient pass, save on fillrate.
		}
		glUniform3fv(effects.forward.camera_position, 1, (float *)&eye_frame);

		//Draw entities
		for (int i = 0; i < LENGTH(pvs); i++)
			draw_drawable(pvs[i]);

		//Draw procedural planet
		for (terrain_tree_drawlist l = terrain_list; l; l = l->next) {
			tri_tile *t = l->tile;
			if (!t->buffered) //Last resort "BUFFER RIGHT NOW", will cause hiccups.
				buffer_tri_tile(t);
			draw_forward(&effects.forward, t->bg, tri_frame_eye_relative);
			checkErrors("After drawing a tri_tile");
		}

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
			glUniform3fv(effects.shadow.gLightPos, 1, (float *)&(point_lights.position[i]));
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
	glUniform3fv(effects.skybox.sun_direction, 1, (float *)&sun_direction);
	glUniform3fv(effects.skybox.sun_color, 1, (float *)&sun_color);
	skybox_frame.t = eye_frame.t;
	//draw_drawable(&d_skybox);

	terrain_tree_drawlist_free(terrain_list);

	checkErrors("After forward junk");
}

void update(float dt)
{
	if (renderer_should_reload) {
		renderer_reload();
		renderer_should_reload = false;
	}

	static float light_time = 0;
	light_time += dt * 0.2;
	point_lights.position[0] = (vec3){10*cos(light_time), 4, 10*sin(light_time)-8};

	if (key_state[SDL_SCANCODE_T]) {
		printf("Ship is at {%f, %f, %f}<%lli, %lli, %lli>. Where would you like to teleport?\n",
			ship.position.t.x, ship.position.t.y, ship.position.t.z,
			ship.sector.x, ship.sector.y, ship.sector.z);
		float x, y, z;
		scanf("%f %f %f %lli %lli %lli", &x, &y, &z, &ship.sector.x, &ship.sector.y, &ship.sector.z);
		ship.position.t = (vec3){x, y, z};
		space_sector_canonicalize(&ship.position.t, &ship.sector);
	}

	float camera_speed = 20.0;
	ship.movable_camera = ship.movable_camera +
		mat3_multvec(eye_frame.a, (vec3){
		(key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A]) * dt * camera_speed,
		(key_state[SDL_SCANCODE_Q] - key_state[SDL_SCANCODE_E]) * dt * camera_speed,
		(key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W]) * dt * camera_speed});

	float controller_max = 32768.0;
	ship = ship_control(dt, (struct controller_input){
		axes[LEFTX]  / controller_max,
		axes[LEFTY]  / controller_max,
		axes[RIGHTX] / controller_max,
		axes[RIGHTY] / controller_max,
		axes[TRIGGERLEFT]  / controller_max,
		axes[TRIGGERRIGHT] / controller_max,
	}, ship);
	space_sector_canonicalize(&ship.position.t, &ship.sector);

	//Translate the camera using WASD.
	eye_frame = ship.locked_camera;
	eye_sector = ship.sector;
	space_sector_canonicalize(&eye_frame.t, &eye_sector);
	tri_frame_eye_relative.a = tri_frame.a;
	tri_frame_eye_relative.t = space_sector_position_relative_to_sector(tri_frame.t, tri_sector, eye_sector);
	point_lights.enabled_for_draw[2] = key_state[SDL_SCANCODE_6];
	
	static float sunscale = 50.0;
	if (key_state[SDL_SCANCODE_EQUALS])
		sunscale += 0.1;
	if (key_state[SDL_SCANCODE_MINUS])
		sunscale -= 0.1;
	sun_color = ambient_color * sunscale;
}
