#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <glalgebra.h>
#include <GL/glew.h>
#include "render.h"
#include "models/models.h"
#include "keyboard.h"
#include "default_settings.h"
#include "buffer_group.h"
#include "controller.h"
#include "deferred_framebuffer.h"
#include "lights.h"
#include "macros.h"
#include "func_list.h"
#include "shader_utils.h"
#include "gl_utils.h"
#include "dynamic_terrain.h"
#include "stars.h"
#include "draw.h"
#include "drawf.h"
#include "drawable.h"

float FOV = M_PI/2.0;
float far_distance = 5000;
int PRIMITIVE_RESTART_INDEX = 0xFFFFFFFF;
const int num_x_tiles = 1;
const int num_z_tiles = 1;
bool DEFERRED_MODE = false;
extern bool draw_light_bounds;

float screen_width = SCREEN_WIDTH;
float screen_height = SCREEN_HEIGHT;

static struct counted_func update_funcs_storage[10];
struct func_list update_func_list = {
	update_funcs_storage,
	0,
	LENGTH(update_funcs_storage)
};

struct buffer_group icosphere_buffers;
//struct terrain ground[num_x_tiles*num_z_tiles];
GLuint gVAO = 0;
amat4 inv_eye_frame;
amat4 eye_frame = {.a = MAT3_IDENT,          .T = {6, 0, 0}};
//Object frames. Need a better system for this.
amat4 ship_frame = {.a = MAT3_IDENT,         .T = {-2.5, 0, -8}};
amat4 newship_frame = {.a = MAT3_IDENT,      .T = {3, 0, -8}};
amat4 teardropship_frame = {.a = MAT3_IDENT, .T = {6, 0, -8}};
amat4 room_frame = {.a = MAT3_IDENT,         .T = {0, -4, -8}};
amat4 grid_frame = {.a = MAT3_IDENT,         .T = {-50, -90, -550}};
amat4 tri_frame  = {.a = MAT3_IDENT,         .T = {-50, -90, -50}};
amat4 big_asteroid_frame = {.a = MAT3_IDENT, .T = {0, -4, -20}};
amat4 skybox_frame; //Set in init_render() and update()
vec3 ambient_color = {{0.01, 0.01, 0.01}};
vec3 sun_direction = {{0.1, 0.8, 0.1}};
vec3 sun_color     = {{0.1, 0.8, 0.1}};
struct point_light_attributes point_lights = {.num_lights = 0};

GLfloat proj_mat[16];
GLfloat proj_view_mat[16];

Drawable d_ship, d_newship, d_teardropship, d_room, d_skybox;

#include "table.h"
//Add this to makefile later.
#include "table.c"

//LISTNODE *terrain_list = NULL;
PDTNODE subdiv_tree = NULL;
float planet_radius;
vec3 planet_center;

// void subdiv_triangle_terrain_list(LISTNODE **list)
// {
// 	LISTNODE *new_list = NULL;
// 	for (LISTNODE *n = *list; n; n = n->next) {
// 		struct terrain *t = (struct terrain *)n->data;
// 		struct terrain *new_t[NUM_TRI_DIVS];
// 		for (int i = 0; i < NUM_TRI_DIVS; i++) {
// 			new_t[i] = malloc(sizeof(struct terrain));
// 			*new_t[i] = new_triangular_terrain(NUM_TRI_ROWS);
// 			LISTNODE *new_n = listnode_new(NULL);
// 			new_n->data = new_t[i];
// 			new_list = list_prepend(new_list, new_n);
// 		}
// 		subdiv_triangle_terrain(t, new_t);
// 		for (int i = 0; i < NUM_TRI_DIVS; i++)
// 			buffer_terrain(new_t[i]);
// 		free_terrain(t);
// 		free(t);
// 	}
// 	list_free(*list);
// 	*list = new_list;
// }

static void init_models()
{
	//In the future, this function should be called in a loop on all entities with the drawable component.
	//Maybe configured from some config file?
	//init_heap_drawable(Drawable *drawable, Draw_func draw, EFFECT *effect, amat4 *frame, int (*buffering_function)(struct buffer_group));
	init_heap_drawable(&d_ship,         draw_forward,        &effects.forward, &ship_frame,         buffer_ship);
	init_heap_drawable(&d_newship,      draw_forward,        &effects.forward, &newship_frame,      buffer_newship);
	init_heap_drawable(&d_teardropship, draw_forward,        &effects.forward, &teardropship_frame, buffer_teardropship);
	init_heap_drawable(&d_room,         draw_forward,        &effects.forward, &room_frame,         buffer_newroom);
	init_heap_drawable(&d_skybox,       draw_skybox_forward, &effects.skybox,  &skybox_frame,       buffer_cube);

	//icosphere_buffers = new_custom_buffer_group(buffer_icosphere, 0, GL_TRIANGLES);

	// float terrain_x = 500;
	// float terrain_z = 500;
	// for (int i = 0; i < num_x_tiles; i++) {
	// 	for (int j = 0; j < num_z_tiles; j++) {
	// 		struct terrain *tmp = &ground[j + i * num_z_tiles];
	// 		*tmp = new_terrain(terrain_x, terrain_z);
	// 		populate_terrain(
	// 			tmp,
	// 			vec3_add(grid_frame.t, (vec3){{(i-(num_x_tiles-1)/2)*terrain_x, 0, (j-(num_z_tiles-1)/2)*terrain_z}}),
	// 			height_map2);
	// 		buffer_terrain(tmp);
	// 	}
	// }

	// struct terrain *t = malloc(sizeof(struct terrain));
	// *t = new_triangular_terrain(NUM_TRI_ROWS);
	vec3 a = vec3_add(tri_frame.t, (vec3){{0.0,               0.0,  TRI_BASE_LEN*(sqrt(3.0)/4.0)}});
	vec3 b = vec3_add(tri_frame.t, (vec3){{-TRI_BASE_LEN/2.0, 0.0, -TRI_BASE_LEN*(sqrt(3.0)/4.0)}});
	vec3 c = vec3_add(tri_frame.t, (vec3){{ TRI_BASE_LEN/2.0, 0.0, -TRI_BASE_LEN*(sqrt(3.0)/4.0)}});
	// populate_triangular_terrain(t, (vec3[3]){a, b, c}, height_map2);
	// buffer_terrain(t);
	// LISTNODE *n = listnode_new(NULL);
	//n->data = t;
	// terrain_list = list_prepend(terrain_list, n);

	planet_radius = TRI_BASE_LEN * cos(M_PI/5.0);
	planet_center = (vec3){{0, -planet_radius, 0}};

	tri_tile t2 = {.is_init = false};
	init_tri_tile(&t2, (vec3[3]){a, b, c}, DEFAULT_NUM_TRI_TILE_ROWS, planet_center, planet_radius);
	gen_tri_tile_vertices_and_normals(&t2, tri_height_map);
	buffer_tri_tile(&t2); //Maybe I don't need to buffer this if my logic in create_drawlist is right.
	subdiv_tree = new_tree(t2, 0);
}

static void deinit_models()
{
	//delete_buffer_group(icosphere_buffers);

	deinit_drawable(&d_ship);
	deinit_drawable(&d_newship);
	deinit_drawable(&d_teardropship);
	deinit_drawable(&d_room);
	deinit_drawable(&d_skybox);

	// for (int i = 0; i < num_x_tiles * num_z_tiles; i++)
	// 	free_terrain(&ground[i]);

	// for (LISTNODE *n = terrain_list; n; n = n->next) {
	// 	free_terrain(n->data);
	// 	free(n->data);
	// }
	// list_free(terrain_list);
	// terrain_list = NULL;

	free_tree(subdiv_tree);
}

static void init_lights()
{
	//                             Position,            color,                atten_c, atten_l, atten_e, intensity
	new_point_light(&point_lights, (vec3){{0, 2, 0}},   (vec3){{1.0, 0.2, 0.2}}, 0.0,     0.0,    1,    20);
	new_point_light(&point_lights, (vec3){{0, 2, 0}},   (vec3){{1.0, 1.0, 1.0}}, 0.0,     0.0,    1,    5);
	new_point_light(&point_lights, (vec3){{0, 2, 0}},   (vec3){{0.8, 0.8, 1.0}}, 0.0,     0.3,    0.04, 0.4);
	new_point_light(&point_lights, (vec3){{10, 4, -7}}, (vec3){{0.4, 1.0, 0.4}}, 0.1,     0.14,   0.07, 0.75);
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
	make_projection_matrix(FOV, screen_width/screen_height, -1, -far_distance, proj_mat, LENGTH(proj_mat));	
}

//Set up everything needed to start rendering frames.
void init_render()
{
	glUseProgram(0);
	glClearDepth(0.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GREATER);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	make_projection_matrix(FOV, screen_width/screen_height, -1, -far_distance, proj_mat, LENGTH(proj_mat));

	init_models();
	init_lights();
	init_stars();

	float skybox_distance = sqrt((far_distance*far_distance)/2);
	skybox_frame.a = mat3_scalemat(skybox_distance, skybox_distance, skybox_distance);
	skybox_frame.t = eye_frame.t;

	glPointSize(3);
	glEnable(GL_PROGRAM_POINT_SIZE);

	glUseProgram(0);
	checkErrors("After init_render");
}

void deinit_render()
{
	deinit_models();
	deinit_stars();
	point_lights.num_lights = 0;
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
	glUniform3fv(effect->uLight_pos, 1, position.A);
	glUniform3fv(effect->uLight_col, 1, lights->color[i].A);
	glUniform4f(effect->uLight_attr, lights->atten_c[i], lights->atten_l[i], lights->atten_e[i], lights->intensity[i]);
}

//Eventually I'll have an algorithm to calculate potential visible set, etc.
//static Drawable *pvs[] = {&d_newship, &d_room};
static Drawable *pvs[] = {};

void render()
{
	DRAWLIST terrain_list = NULL;
	subdivide_tree(subdiv_tree, eye_frame.t);
	create_drawlist(subdiv_tree, &terrain_list);
	prune_tree(subdiv_tree);

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
	glDepthFunc(GL_GREATER);

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
			glUniform3fv(effects.forward.uLight_col, 1, sun_color.A);
			glUniform3fv(effects.forward.uLight_pos, 1, sun_direction.A);
		} else {
			glDrawBuffer(GL_NONE); //Disable drawing to the color buffer if no ambient pass, save on fillrate.
		}
		glUniform3fv(effects.forward.camera_position, 1, eye_frame.T);

		//Draw entities
		for (int i = 0; i < LENGTH(pvs); i++)
			draw_drawable(pvs[i]);

		//Draw terrain, which receives, but does not cast, stencil buffer shadows.
		// for (int i = 0; i < num_x_tiles*num_z_tiles; i++)
		// 	draw_forward(&effects.forward, ground[i].bg, grid_frame);

		// for (LISTNODE *n = terrain_list; n; n = n->next)
		// 	draw_forward(&effects.forward, ((struct terrain *)n->data)->bg, tri_frame);
		for (DRAWLIST l = terrain_list; l; l = l->next) {
			draw_forward(&effects.forward, l->t->bg, tri_frame);
			checkErrors("After drawing a tri_tile");
		}

		glUseProgram(effects.forward.handle);
		checkErrors("After drawing into depth");
		glUniform1i(effects.forward.ambient_pass, 0);
	}

	//For each light, draw potential occluders to set the stencil buffer.
	//Then draw the scene, accumulating non-occluded light onto the models additively.
	for (int i = 0; i < point_lights.num_lights; i++) {
		//Render shadow volumes into the stencil buffer.
		if (point_lights.shadowing[i]) {
			glEnable(GL_DEPTH_CLAMP);
			glDepthFunc(GL_GREATER);
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
			glUniform3fv(effects.shadow.gLightPos, 1, point_lights.position[i].A);
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

			glDisable(GL_BLEND);
			checkErrors("After drawing shadowed");
		}
		glClear(GL_STENCIL_BUFFER_BIT);
	}
	glDisable(GL_BLEND);
	glDepthFunc(GL_GREATER);

	glUseProgram(effects.skybox.handle);
	glUniform3fv(effects.skybox.sun_direction, 1, sun_direction.A);
	glUniform3fv(effects.skybox.sun_color, 1, sun_color.A);
	skybox_frame.t = eye_frame.t;
	//draw_drawable(&d_skybox);

	drawlist_free(terrain_list);

	checkErrors("After forward junk");
}

void update(float dt)
{
	func_list_call(&update_func_list);
	static float light_time = 0;
	static amat4 velocity = AMAT4_IDENT;
	static amat4 ship_cam = {.a = MAT3_IDENT, .T = {0, 4, 8}}; //Camera position, relative to the ship's frame.
	light_time += dt * 0.2;
	point_lights.position[0] = vec3_new(10*cos(light_time), 4, 10*sin(light_time)-8);

	float ts = 1/300000.0;
	float rs = 1/600000.0;
	// if (key_state[SDL_SCANCODE_9])
	// 	for (int i = 0; i < num_x_tiles*num_z_tiles; i++)
	// 		recalculate_terrain_normals_cheap(&ground[i]);
	// if (key_state[SDL_SCANCODE_8])
	// 	for (int i = 0; i < num_x_tiles*num_z_tiles; i++)
	// 		erode_terrain(&ground[i], 100);
	// if (key_state[SDL_SCANCODE_8] || key_state[SDL_SCANCODE_9])
	// 	for (int i = 0; i < num_x_tiles*num_z_tiles; i++)
	// 		buffer_terrain(&ground[i]);

	// static bool divide = false;
	// if (key_state[SDL_SCANCODE_4]) {
	// 	divide = true;
	// } else {
	// 	if (divide) {
	// 		subdiv_triangle_terrain_list(&terrain_list);
	// 		divide = false;
	// 	}
	// }

	//If you're moving forward, turn the light on to show it.
	//point_lights.enabled_for_draw[2] = (axes[LEFTY] < 0)?true:false;
	point_lights.enabled_for_draw[2] = key_state[SDL_SCANCODE_6];
	float camera_speed = 20.0;
	float ship_speed = 80.0;

	//Translate the camera using WASD.
	ship_cam.t = vec3_add(ship_cam.t,
		mat3_multvec(eye_frame.a, (vec3){{
		(key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A]) * dt * camera_speed,
		(key_state[SDL_SCANCODE_Q] - key_state[SDL_SCANCODE_E]) * dt * camera_speed,
		(key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W]) * dt * camera_speed}}));

	//Set the ship's acceleration using the controller axes.
	vec3 acceleration = mat3_multvec(ship_frame.a, (vec3){{
		axes[LEFTX] * ts,
		0,
		axes[LEFTY] * ts}});

	//Set the ship's acceleration using the arrow keys.
	acceleration = mat3_multvec(ship_frame.a, (vec3){{
		(key_state[SDL_SCANCODE_RIGHT] - key_state[SDL_SCANCODE_LEFT]) * dt * ship_speed,
		(key_state[SDL_SCANCODE_PERIOD] - key_state[SDL_SCANCODE_SLASH]) * dt * ship_speed,
		(key_state[SDL_SCANCODE_DOWN] - key_state[SDL_SCANCODE_UP]) * dt * ship_speed}});

	//Angular velocity is currently determined by how much each axis is deflected.
	float angle = -axes[RIGHTX]*rs;
	velocity.a = mat3_rot(mat3_rotmat(0, 0, 1, sin(angle), cos(angle)), 1, 0, 0, sin(-angle), cos(-angle));

	//Add our acceleration to our velocity to change our speed.
	//velocity.t = vec3_add(vec3_scale(velocity.t, dt), acceleration);

	//Linear motion just sets the velocity directly.
	velocity.t = vec3_scale(acceleration, dt*ship_speed);

	//Rotate the ship by applying the angular velocity to the angular orientation.
	ship_frame.a = mat3_mult(ship_frame.a, velocity.a);

	//Move the ship by applying the velocity to the position.
	ship_frame.t = vec3_add(ship_frame.t, velocity.t);

	//The eye should be positioned 2 up and 8 back relative to the ship's frame.
	float alpha = 0.1;
	eye_frame.t = vec3_lerp(eye_frame.t, amat4_multpoint(ship_frame, ship_cam.t), alpha);
	static vec3 eye_target = {{0.0, 0.0, 0.0}};
	eye_target = vec3_lerp(eye_target, amat4_multpoint(ship_frame, (vec3){{0, 0, -4}}), alpha);

	//The eye should look from itself to a point in front of the ship, and its "up" should be "up" from the ship's orientation.
	eye_frame.a = mat3_lookat(
		eye_frame.t,
		eye_target,
		mat3_multvec(ship_frame.a, (vec3){{0, 1, 0}}));

	//Move the engine light to just behind the ship.
	point_lights.position[2] = amat4_multpoint(ship_frame, (vec3){{0, 0, 6}});

	static float sunscale = 50.0;
	if (key_state[SDL_SCANCODE_EQUALS])
		sunscale += 0.1;
	if (key_state[SDL_SCANCODE_MINUS])
		sunscale -= 0.1;
	sun_color = vec3_scale(ambient_color, sunscale);
}
