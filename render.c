#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <glalgebra.h>
#include <GL/glew.h>
#include "render.h"
#include "models/models.h"
#include "shaders/shaders.h"
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
#include "procedural_terrain.h"
#include "stars.h"

float FOV = M_PI/2.5;
float far_distance = 2000;
int PRIMITIVE_RESTART_INDEX = 0xFFFFFFFF;
int num_x_tiles = 1;
int num_z_tiles = 1;
bool DEFERRED_MODE = false;
extern bool draw_light_bounds;

static struct counted_func update_funcs_storage[10];
struct func_list update_func_list = {
	update_funcs_storage,
	0,
	LENGTH(update_funcs_storage)
};

//Buffer group declarations. Need a better system for this.
struct buffer_group newship_buffers;
struct buffer_group teardropship_buffers;
struct buffer_group ship_buffers;
struct buffer_group ball_buffers;
struct buffer_group thrust_flare_buffers;
struct buffer_group room_buffers;
struct buffer_group icosphere_buffers;
struct buffer_group big_asteroid_buffers;
struct buffer_group grid_buffers;
struct buffer_group cube_buffers;
struct buffer_group triangle_buffers;
struct terrain ground[5*5];
GLuint gVAO = 0;
amat4 inv_eye_frame;
amat4 eye_frame = {.a = MAT3_IDENT, .T = {6, 0, 0}};
//Object frames. Need a better system for this.
amat4 ship_frame = {.a = MAT3_IDENT, .T = {-2.5, 0, -8}};
amat4 newship_frame = {.a = MAT3_IDENT, .T = {3, 0, -8}};
amat4 teardropship_frame = {.a = MAT3_IDENT, .T = {6, 0, -8}};
amat4 room_frame = {.a = MAT3_IDENT, .T = {0, -4, -8}};
amat4 grid_frame = {.a = MAT3_IDENT, .T = {-50, -50, -50}};
amat4 big_asteroid_frame = {.a = MAT3_IDENT, .T = {0, -4, -20}};
static vec3 skybox_scale;
static vec3 ambient_color = {{0.01, 0.01, 0.01}};
static vec3 sun_direction = {{0.1, 0.8, 0.1}};
static vec3 sun_color     = {{0.1, 0.8, 0.1}};
struct point_light_attributes point_lights = {.num_lights = 0};

GLfloat proj_mat[16];
GLfloat proj_view_mat[16];

int buffer_triangle(struct buffer_group bg)
{
	GLfloat positions[] = {
		1.0, 0.0, 0.0,
		cos(2*M_PI/3), sin(2*M_PI/3), 0,
		cos(4*M_PI/3), sin(4*M_PI/3), 0
	};
	GLuint indices[] = {
		0, 1, 2
	};
	glBindBuffer(GL_ARRAY_BUFFER, bg.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	return sizeof(indices)/sizeof(indices[0]);
}

static void init_models()
{
	ship_buffers = new_buffer_group(buffer_ship, &forward_program);
	newship_buffers = new_buffer_group(buffer_newship, &forward_program);
	teardropship_buffers = new_buffer_group(buffer_teardropship, &forward_program);
	ball_buffers = new_buffer_group(buffer_ball, &forward_program);
	thrust_flare_buffers = new_buffer_group(buffer_thrust_flare, &forward_program);
	icosphere_buffers = new_custom_buffer_group(buffer_icosphere, 0, GL_TRIANGLES);
	triangle_buffers = new_custom_buffer_group(buffer_triangle, 0, GL_TRIANGLES);
	room_buffers = new_buffer_group(buffer_newroom, &forward_program);
	//big_asteroid_buffers = new_buffer_group(buffer_big_asteroid, &forward_program);
	cube_buffers = new_buffer_group(buffer_cube, &skybox_program);
	//grid_buffers = buffer_grid(128, 128);
	float terrain_x = 300;
	float terrain_y = 300;
	for (int i = 0; i < num_x_tiles; i++) {
		for (int j = 0; j < num_z_tiles; j++) {
			struct terrain *tmp = &ground[j + i * 5];
			*tmp = new_terrain(terrain_x+1, terrain_y+1);
			populate_terrain(tmp, (vec3){{(i-num_x_tiles/2)*terrain_x, 0, (j-num_z_tiles/2)*terrain_y}}, height_map2);
			buffer_terrain(tmp);
		}
	}
	// height_map_test = new_terrain(100, 100);
	// populate_terrain(&height_map_test, (vec3){{0,0,0}}, height_map1, height_map_normal1);
	// buffer_terrain(&height_map_test);
}

static void deinit_models()
{
	delete_buffer_group(newship_buffers);
	delete_buffer_group(teardropship_buffers);
	delete_buffer_group(ball_buffers);
	delete_buffer_group(thrust_flare_buffers);
	delete_buffer_group(icosphere_buffers);
	delete_buffer_group(room_buffers);
	delete_buffer_group(triangle_buffers);
	//delete_buffer_group(grid_buffers);
	//free_terrain(&height_map_test);
	for (int i = 0; i < 5 * 5; i++)
		free_terrain(&ground[i]);
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
	point_lights.shadowing[2] = true;
	point_lights.shadowing[3] = true;

	for (int i = 0; i < point_lights.num_lights; i++) {
		printf("Light %i radius is %f\n", i, point_lights.radius[i]);
	}
}

// //Later I should put the projection matrix in a uniform block
// static void send_projection_matrix()
// {
// 	glUseProgram(skybox_program.handle);
// 	glUniformMatrix4fv(skybox_program.projection_matrix, 1, GL_TRUE, proj_mat);
// 	glUseProgram(forward_program.handle);
// 	glUniformMatrix4fv(forward_program.projection_matrix, 1, GL_TRUE, proj_mat);
// 	glUseProgram(outline_program.handle);
// 	glUniformMatrix4fv(outline_program.projection_matrix, 1, GL_TRUE, proj_mat);
// 	glUseProgram(shadow_program.handle);
// 	glUniformMatrix4fv(outline_program.projection_matrix, 1, GL_TRUE, proj_mat);
// }

void handle_resize(int width, int height)
{
	glViewport(0, 0, width, height);
	make_projection_matrix(FOV, (float)width/(float)height, -1, -far_distance, proj_mat, LENGTH(proj_mat));	
	//send_projection_matrix();
}

//Set up everything needed to start rendering frames.
void init_render()
{
	glUseProgram(0);
	glClearDepth(0.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GREATER);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	make_projection_matrix(FOV, (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, -1, -far_distance, proj_mat, LENGTH(proj_mat));

	init_models();
	init_lights();
	init_stars();

	float skybox_distance = sqrt((far_distance*far_distance)/2);
	skybox_scale = (vec3){{skybox_distance, skybox_distance, skybox_distance}};
	//send_projection_matrix();

	glPointSize(3);
	glEnable(GL_PROGRAM_POINT_SIZE);

	glUseProgram(0);
	checkErrors("After init");
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

static void draw_skybox_forward(struct shader_prog *program, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat tmp[16];
	GLfloat model_matrix_buf[16];
	amat4_to_array(model_matrix, model_matrix_buf);
	amat4_buf_mult(proj_view_mat, model_matrix_buf, tmp);
	//Send model_view_projection_matrix.
	glUniformMatrix4fv(program->model_view_projection_matrix, 1, GL_TRUE, tmp);
	//Send model_matrix
	glUniformMatrix4fv(program->model_matrix, 1, GL_TRUE, model_matrix_buf);
	glUniform3fv(program->camera_position, 1, eye_frame.T);
	glUniform3fv(program->sun_direction, 1, sun_direction.A);
	//Draw!
	glDrawElements(GL_TRIANGLES, bg.index_count, GL_UNSIGNED_INT, NULL);
}

static void draw_forward(struct shader_prog *program, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat tmp[16];
	GLfloat model_matrix_buf[16];
	amat4_to_array(model_matrix, model_matrix_buf);
	amat4_buf_mult(proj_view_mat, model_matrix_buf, tmp);
	//Send model_view_projection_matrix.
	glUniformMatrix4fv(program->model_view_projection_matrix, 1, GL_TRUE, tmp);
	//Send model_matrix
	glUniformMatrix4fv(program->model_matrix, 1, GL_TRUE, model_matrix_buf);
	//Send normal_model_view_matrix
	mat3_vec3_to_array(mat3_transp(model_matrix.a), (vec3){{0, 0, 0}}, tmp);
	glUniformMatrix4fv(program->model_view_normal_matrix, 1, GL_TRUE, tmp);
	glDrawElements(bg.primitive_type, bg.index_count, GL_UNSIGNED_INT, NULL);
}

static void draw_wireframe(struct shader_prog *program, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat tmp[16];
	GLfloat model_matrix_buf[16];
	amat4_to_array(model_matrix, model_matrix_buf);
	amat4_buf_mult(proj_view_mat, model_matrix_buf, tmp);
	//Send model_view_projection_matrix.
	glUniformMatrix4fv(program->model_view_projection_matrix, 1, GL_TRUE, tmp);
	glDrawElements(bg.primitive_type, bg.index_count, GL_UNSIGNED_INT, NULL);
}

static void draw_forward_adjacent(struct shader_prog *program, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat tmp[16];
	GLfloat model_matrix_buf[16];
	amat4_to_array(model_matrix, model_matrix_buf);
	amat4_buf_mult(proj_view_mat, model_matrix_buf, tmp);
	//Send model_view_projection_matrix.
	glUniformMatrix4fv(program->model_view_projection_matrix, 1, GL_TRUE, tmp);
	//Send model_matrixh
	glUniformMatrix4fv(program->model_matrix, 1, GL_TRUE, model_matrix_buf);
	//Send normal_model_view_matrix
	mat3_vec3_to_array(mat3_transp(model_matrix.a), (vec3){{0, 0, 0}}, tmp);
	glUniformMatrix4fv(program->model_view_normal_matrix, 1, GL_TRUE, tmp);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.aibo);
	glDrawElements(GL_TRIANGLES_ADJACENCY, bg.index_count*2, GL_UNSIGNED_INT, NULL);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);
	checkErrors("After drawing with aibo");
}

static void forward_update_point_light(struct shader_prog *program, struct point_light_attributes *lights, int i)
{
	vec3 position = lights->position[i];
	glUniform3fv(program->uLight_pos, 1, position.A);
	glUniform3fv(program->uLight_col, 1, lights->color[i].A);
	glUniform4f(program->uLight_attr, lights->atten_c[i], lights->atten_l[i], lights->atten_e[i], lights->enabled_for_draw[i]?lights->intensity[i]:0.0);
}


void render_depth(struct shader_prog *program, struct buffer_group *bgs[], amat4 *frames[], int len)
{
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glDrawBuffer(GL_NONE);
	glUseProgram(program->handle);
	for (int i = 0; i < len; i++) {
		draw_forward(program, *bgs[i], *frames[i]);
	}
	checkErrors("After drawing into depth");
}

/*
START LOD PLANET STUFF
*/
#define VEC3_3AVERAGE(v1, v2, v3) vec3_scale(vec3_add(vec3_add(v1,v2), v3), 1.0/3.0)
#define VEC3_2AVERAGE(v1, v2) vec3_scale(vec3_add(v1,v2),0.5)
#define CLAMP(val, low, high) val<low?low:(val>high?high:val)

static void draw_triangle(vec3 center, vec3 p1, vec3 p2, vec3 p3)
{
	float s = vec3_mag(vec3_sub(p1, p2));
	vec3 triangle_center = VEC3_3AVERAGE(p1, p2, p3);
	draw_wireframe(&wireframe_program, triangle_buffers, (amat4){.a = mat3_scale(mat3_lookat(triangle_center, center, (vec3){{1.0, 0.0, 0.0}}), s, s, s), .t = triangle_center});
}

static void draw_recursive(vec3 p1, vec3 p2, vec3 p3, vec3 center, float size);
static void draw_recursive(vec3 p1, vec3 p2, vec3 p3, vec3 center, float size)
{
	float ratio = 1;//gui.screen[0].slider["lod.ratio"].val; // default : 1
	float minsize = 0.01; //gui.screen[0].slider["detail"].val;  // default : 0.01

	double dot = vec3_dot(VEC3_3AVERAGE(p1, p2, p3), center);
	double dist = acos(CLAMP(dot, -1, 1)) / M_PI;

	if (dist > 0.5) return;//culling

	if (dist > ratio * size || size < minsize) 
	{ 
		draw_triangle(center, p1, p2, p3); 
		return; 
	}

	// Recurse
	vec3 p[6] = { p1, p2, p3, VEC3_2AVERAGE(p1, p2), VEC3_2AVERAGE(p2, p3), VEC3_2AVERAGE(p3, p1)};
	int idx[12] = { 0, 3, 5, 5, 3, 4, 3, 1, 4, 5, 4, 2 };

	for (int i = 0; i < 4; i++) {
		draw_recursive(
			vec3_normalize(p[idx[3 * i + 0]]), 
			vec3_normalize(p[idx[3 * i + 1]]),
			vec3_normalize(p[idx[3 * i + 2]]),
			center, size/2);
	}
}

static void draw(vec3 center)
{
	glDisable(GL_CULL_FACE);
	// create icosahedron
	float t = (1.0 + sqrt(5.0)) / 2.0;

	vec3 p[] = { 
		{{ -1, t, 0 }}, {{ 1, t, 0 }}, {{ -1, -t, 0 }}, {{ 1, -t, 0 }},
		{{ 0, -1, t }}, {{ 0, 1, t }}, {{ 0, -1, -t }}, {{ 0, 1, -t }},
		{{ t, 0, -1 }}, {{ t, 0, 1 }}, {{ -t, 0, -1 }}, {{ -t, 0, 1 }},
	};
	GLuint idx[] = { 
		0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11,
		1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 10, 7, 6, 7, 1, 8,
		3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9,
		4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1
	};

	for (int i = 0; i < LENGTH(idx)/3; i++) {
		draw_recursive(
			vec3_normalize(p[idx[i * 3 + 0]]), // triangle point 1
			vec3_normalize(p[idx[i * 3 + 1]]), // triangle point 2
			vec3_normalize(p[idx[i * 3 + 2]]), // triangle point 3
			center, 1);
	}
	glEnable(GL_CULL_FACE);
}

/*
END LOD PLANET STUFF
*/

void render()
{
	{
		inv_eye_frame = amat4_inverse(eye_frame); //Only need to do this once per frame.
		float tmp[16];
		amat4_to_array(inv_eye_frame, tmp);
		amat4_buf_mult(proj_mat, tmp, proj_view_mat);
	}

	static struct buffer_group *pvs[] = {
		//&room_buffers,
		//&teardropship_buffers,
		//&ship_buffers,
		//&newship_buffers,
		//&height_map_test.bg
	};
	static struct buffer_group *pvs_shadowers[] = {
		//&room_buffers,
		//&teardropship_buffers,
		//&newship_buffers
		//&ship_buffers
	};
	static amat4 *pvs_frames[] = {
		//&room_frame,
		//&newship_frame,
		//&teardropship_frame,
		//&ship_frame,
		//&grid_frame
	};
	glDepthMask(GL_TRUE);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_GREATER);
	int zpass = key_state[SDL_SCANCODE_7];
	int apass = !key_state[SDL_SCANCODE_5];
	//Render into the depth buffer.
	{
		//glDrawBuffer(GL_NONE);
		glUseProgram(forward_program.handle);
		if (apass) {
			glUniform1i(forward_program.ambient_pass, 1);
			glUniform3fv(forward_program.uLight_col, 1, sun_color.A);
			glUniform3fv(forward_program.uLight_pos, 1, sun_direction.A);
		}
		glUniform3fv(forward_program.camera_position, 1, eye_frame.T);
		//Draw entities
		for (int i = 0; i < LENGTH(pvs); i++)
			draw_forward(&forward_program, *pvs[i], *pvs_frames[i]);
		//Draw terrain
		for (int i = 0; i < num_x_tiles*num_z_tiles; i++)
			draw_forward(&forward_program, ground[i].bg, grid_frame);
			//draw_forward(&forward_program, ground[i].bg, (amat4){.a = grid_frame.a, .t = ground[i].pos});
		glUseProgram(wireframe_program.handle);
		//draw(vec3_sub((vec3){{0, 0, 0}}, eye_frame.t));
		glUseProgram(forward_program.handle);
		checkErrors("After drawing into depth");
		glUniform1i(forward_program.ambient_pass, 0);
	}
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
			glUseProgram(shadow_program.handle);
			glUniform3fv(shadow_program.gLightPos, 1, point_lights.position[i].A);
			//glUniform3fv(shadow_program.light_color, 1, point_lights.color[i].A);
			glUniform1i(shadow_program.zpass, zpass);
			checkErrors("After updating zpass uniform");
			//glDrawBuffer(GL_BACK);
			for (int i = 0; i < LENGTH(pvs_shadowers); i++)
				draw_forward_adjacent(&shadow_program, *pvs_shadowers[i], *pvs_frames[i]);
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
			glUseProgram(forward_program.handle);
			forward_update_point_light(&forward_program, &point_lights, i);
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_ONE, GL_ONE);
			glDepthFunc(GL_EQUAL);
			checkErrors("Before forward draw shadowed");
			for (int i = 0; i < LENGTH(pvs); i++) {
				draw_forward(&forward_program, *pvs[i], *pvs_frames[i]);
				checkErrors("After forward draw shadowed");
			}
			glDisable(GL_BLEND);
			checkErrors("After drawing shadowed");
		}
		glClear(GL_STENCIL_BUFFER_BIT);
	}
	glDisable(GL_BLEND);
	glDepthFunc(GL_GREATER);
	glUseProgram(skybox_program.handle);
	glUniform3fv(skybox_program.sun_direction, 1, sun_direction.A);
	glUniform3fv(skybox_program.sun_color, 1, sun_color.A);
	amat4 skybox_frame = {
	.a = mat3_scalemat(skybox_scale.x, skybox_scale.y, skybox_scale.z),
	.t = eye_frame.t};
	draw_skybox_forward(&skybox_program, cube_buffers, skybox_frame);
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
	if (key_state[SDL_SCANCODE_9])
		for (int i = 0; i < num_x_tiles*num_z_tiles; i++)
			recalculate_terrain_normals(&ground[i]);
	if (key_state[SDL_SCANCODE_8])
		for (int i = 0; i < num_x_tiles*num_z_tiles; i++)
			erode_terrain(&ground[i], 100);
	if (key_state[SDL_SCANCODE_8] || key_state[SDL_SCANCODE_9])
		for (int i = 0; i < num_x_tiles*num_z_tiles; i++)
			buffer_terrain(&ground[i]);

	//If you're moving forward, turn the light on to show it.
	//point_lights.enabled_for_draw[2] = (axes[LEFTY] < 0)?true:false;
	point_lights.enabled_for_draw[2] = key_state[SDL_SCANCODE_6];
	float camera_speed = 20;
	float ship_speed = 20;
	// if (key_state[SDL_SCANCODE_2])
	// 	draw_light_bounds = true;
	// else
	// 	draw_light_bounds = false;
	// point_lights.position[3] = vec3_add(point_lights.position[3], (vec3){{
	// 	(key_state[SDL_SCANCODE_RIGHT] - key_state[SDL_SCANCODE_LEFT]) * dt * camera_speed,
	// 	0,
	// 	(key_state[SDL_SCANCODE_DOWN] - key_state[SDL_SCANCODE_UP]) * dt * camera_speed}});
	//Translate the camera using WASD.
	ship_cam.t = vec3_add(ship_cam.t,
		mat3_multvec(eye_frame.a, (vec3){{
		(key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A]) * dt * camera_speed,
		(key_state[SDL_SCANCODE_Q] - key_state[SDL_SCANCODE_E]) * dt * camera_speed,
		(key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W]) * dt * camera_speed}}));

	//grid_frame.t = point_lights.position[3];
	//Set the ship's acceleration using the controller axes.
	vec3 acceleration = mat3_multvec(ship_frame.a, (vec3){{
		axes[LEFTX] * ts,
		0,
		axes[LEFTY] * ts}});
	//Set the ship's acceleration using the arrow keys.
	acceleration = mat3_multvec(ship_frame.a, (vec3){{
		(key_state[SDL_SCANCODE_RIGHT] - key_state[SDL_SCANCODE_LEFT]) * dt * ship_speed,
		0,
		(key_state[SDL_SCANCODE_DOWN] - key_state[SDL_SCANCODE_UP]) * dt * ship_speed}});
	//Angular velocity is currently determined by how much each axis is deflected.
	float angle = -axes[RIGHTX]*rs;
	velocity.a = mat3_rot(mat3_rotmat(0, 0, 1, sin(angle), cos(angle)), 1, 0, 0, sin(-angle), cos(-angle));
	//Add our acceleration to our velocity to change our speed.
	//velocity.t = vec3_add(vec3_scale(velocity.t, dt), acceleration);
	//Linear motion just sets the velocity directly.
	velocity.t = vec3_scale(acceleration, dt*ship_speed);
	//velocity.t = vec3_add(velocity.t, acceleration);
	//Rotate the ship by applying the angular velocity to the angular orientation.
	ship_frame.a = mat3_mult(ship_frame.a, velocity.a);
	//Move the ship by applying the velocity to the position.
	ship_frame.t = vec3_add(ship_frame.t, velocity.t);
	//The eye should be positioned 2 up and 8 back relative to the ship's frame.
	float alpha = 0.8;
	//eye_frame.t = amat4_multpoint(ship_frame, ship_cam.t);
	eye_frame.t = vec3_lerp(eye_frame.t, amat4_multpoint(ship_frame, ship_cam.t), alpha);
	static vec3 eye_target = {{0.0, 0.0, 0.0}};
	//eye_target = vec3_lerp(eye_target, amat4_multpoint(ship_frame, (vec3){{0, 0, -4}}), alpha);
	eye_target = amat4_multpoint(ship_frame, (vec3){{0, 0, -4}}); //The camera points a little bit ahead of the ship.
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
