#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <GL/glew.h>
#include "render.h"
#include "math/affine_matrix4.h"
#include "math/matrix3.h"
#include "math/vector3.h"
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

#define FOV M_PI/2.5
int PRIMITIVE_RESTART_INDEX = 0xFFFF;

#define DEFERRED_MODE false

static struct counted_func update_funcs_storage[10];
struct func_list update_func_list = {
	update_funcs_storage,
	0,
	LENGTH(update_funcs_storage)
};

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
GLuint gVAO = 0;
extern bool draw_light_bounds;
AMAT4 inv_eye_frame;
AMAT4 eye_frame = {.a = MAT3_IDENT, .T = {0, 0, 0}};
AMAT4 ship_frame = {.a = MAT3_IDENT, .T = {0, 0, -8}};
AMAT4 room_frame = {.a = MAT3_IDENT, .T = {0, -4, -8}};
AMAT4 grid_frame = {.a = MAT3_IDENT, .T = {-30, -3, -30}};
AMAT4 big_asteroid_frame = {.a = MAT3_IDENT, .T = {0, -4, -20}};
static VEC3 skybox_scale;
struct point_light_attributes point_lights = {.num_lights = 0};

GLfloat proj_mat[16];
extern void init_deferred_render();
extern void deinit_deferred_render();

static void init_forward_render()
{

}

static void init_models()
{
	ship_buffers = new_buffer_group(buffer_ship, &forward_program);
	newship_buffers = new_buffer_group(buffer_newship, &forward_program);
	teardropship_buffers = new_buffer_group(buffer_teardropship, &forward_program);
	ball_buffers = new_buffer_group(buffer_ball, &forward_program);
	thrust_flare_buffers = new_buffer_group(buffer_thrust_flare, &forward_program);
	icosphere_buffers = new_custom_buffer_group(buffer_icosphere, 0);
	room_buffers = new_buffer_group(buffer_room, &forward_program);
	//big_asteroid_buffers = new_buffer_group(buffer_big_asteroid, &forward_program);
	cube_buffers = new_buffer_group(buffer_cube, &skybox_program);
	grid_buffers = buffer_grid(128, 128);
}

static void deinit_models()
{
	delete_buffer_group(newship_buffers);
	delete_buffer_group(teardropship_buffers);
	delete_buffer_group(ball_buffers);
	delete_buffer_group(thrust_flare_buffers);
	delete_buffer_group(icosphere_buffers);
	delete_buffer_group(room_buffers);
	delete_buffer_group(grid_buffers);
}

static void init_lights()
{
	//                             Position,            color,                atten_c, atten_l, atten_e, intensity
	new_point_light(&point_lights, (VEC3){{0, 0, 0}},   (VEC3){{1.0, 0.2, 0.2}}, 0.0,     0.0,    1,    20);
	new_point_light(&point_lights, (VEC3){{0, 0, 0}},   (VEC3){{1.0, 1.0, 1.0}}, 0.0,     0.0,    1,    50);
	new_point_light(&point_lights, (VEC3){{0, 0, 0}},   (VEC3){{0.8, 0.8, 1.0}}, 0.0,     0.3,    0.04, 0.4);
	new_point_light(&point_lights, (VEC3){{10, 2, -7}}, (VEC3){{0.4, 1.0, 0.4}}, 0.1,     0.14,   0.07, 0.75);

	for (int i = 0; i < point_lights.num_lights; i++) {
		printf("Light %i radius is %f\n", i, point_lights.radius[i]);
	}
}

//Set up everything needed to start rendering frames.
void init_render()
{
	glGenVertexArrays(1, &gVAO);
	glBindVertexArray(gVAO);

	glUseProgram(0);
	glClearDepth(0.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GREATER);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float far_distance = 2000;
	make_projection_matrix(FOV, (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, -1, -far_distance, proj_mat, LENGTH(proj_mat));

	init_forward_render();
	init_models();
	init_lights();
	init_stars();

	float skybox_distance = sqrt((far_distance*far_distance)/2);
	skybox_scale = (VEC3){{skybox_distance, skybox_distance, skybox_distance}};
	//Setup unchanging deferred uniforms.
	glUseProgram(skybox_program.handle);
	glUniformMatrix4fv(skybox_program.projection_matrix, 1, GL_TRUE, proj_mat);
	glUseProgram(forward_program.handle);
	glUniformMatrix4fv(forward_program.projection_matrix, 1, GL_TRUE, proj_mat);
	glUseProgram(outline_program.handle);
	glUniformMatrix4fv(outline_program.projection_matrix, 1, GL_TRUE, proj_mat);
	glUseProgram(shadow_program.handle);
	glUniformMatrix4fv(outline_program.projection_matrix, 1, GL_TRUE, proj_mat);
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

static void draw_skybox_forward(struct shader_prog *program, struct buffer_group bg, AMAT4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat mvm_buf[16];
	AMAT4 model_view_matrix = amat4_mult(inv_eye_frame, model_matrix);
	//Send model_view_matrix.
	amat4_to_array(mvm_buf, LENGTH(mvm_buf), model_view_matrix);
	glUniformMatrix4fv(program->model_view_matrix, 1, GL_TRUE, mvm_buf);
	//Send model_matrix
	amat4_to_array(mvm_buf, LENGTH(mvm_buf), model_matrix);
	glUniformMatrix4fv(program->model_matrix, 1, GL_TRUE, mvm_buf);
	glUniform3fv(program->camera_position, 1, eye_frame.T);
	//Draw!
	glDrawElements(GL_TRIANGLES, bg.index_count, GL_UNSIGNED_INT, NULL);
}

static void draw_forward(struct shader_prog *program, struct buffer_group bg, AMAT4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat mvm_buf[16];
	AMAT4 model_view_matrix = amat4_mult(inv_eye_frame, model_matrix);
	//Send model_view_matrix.
	amat4_to_array(mvm_buf, LENGTH(mvm_buf), model_view_matrix);
	glUniformMatrix4fv(program->model_view_matrix, 1, GL_TRUE, mvm_buf);
	//Send model_matrix
	amat4_to_array(mvm_buf, LENGTH(mvm_buf), model_matrix);
	glUniformMatrix4fv(program->model_matrix, 1, GL_TRUE, mvm_buf);
	//Send normal_model_view_matrix
	mat3_vec3_to_array(mvm_buf, LENGTH(mvm_buf), mat3_transp(model_matrix.a), (VEC3){{0, 0, 0}});
	glUniformMatrix4fv(program->model_view_normal_matrix, 1, GL_TRUE, mvm_buf);
	glDrawElements(bg.primitive_type, bg.index_count, GL_UNSIGNED_INT, NULL);
}

static void draw_forward_adjacent(struct shader_prog *program, struct buffer_group bg, AMAT4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat mvm_buf[16];
	AMAT4 model_view_matrix = amat4_mult(inv_eye_frame, model_matrix);
	//Send model_view_matrix.
	amat4_to_array(mvm_buf, LENGTH(mvm_buf), model_view_matrix);
	glUniformMatrix4fv(program->model_view_matrix, 1, GL_TRUE, mvm_buf);
	//Send model_matrix
	amat4_to_array(mvm_buf, LENGTH(mvm_buf), model_matrix);
	glUniformMatrix4fv(program->model_matrix, 1, GL_TRUE, mvm_buf);
	//Send normal_model_view_matrix
	mat3_vec3_to_array(mvm_buf, LENGTH(mvm_buf), mat3_transp(model_matrix.a), (VEC3){{0, 0, 0}});
	glUniformMatrix4fv(program->model_view_normal_matrix, 1, GL_TRUE, mvm_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.aibo);
	glDrawElements(GL_TRIANGLES_ADJACENCY, bg.index_count*2, GL_UNSIGNED_INT, NULL);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);
	checkErrors("After drawing with aibo");
}

static void forward_update_point_lights(struct shader_prog *program, struct point_light_attributes *lights)
{
	char buf[50];
	int num_lights = lights->num_lights;
	for (int i = 0; i < num_lights; i++) {
		VEC3 position = lights->position[i];
		sprintf(buf, "uLight_pos[%i]", i);
		glUniform3fv(glGetUniformLocation(program->handle, buf), 1, position.A);
		sprintf(buf, "uLight_col[%i]", i);
		glUniform3fv(glGetUniformLocation(program->handle, buf), 1, lights->color[i].A);
		sprintf(buf, "uLight_attr[%i]", i);
		glUniform4f(glGetUniformLocation(program->handle, buf), lights->atten_c[i], lights->atten_l[i], lights->atten_e[i], lights->enabled_for_draw[i]?lights->intensity[i]:0.0);
	}
}

void render()
{
	inv_eye_frame = amat4_inverse(eye_frame); //Only need to do this once per frame.

	if (DEFERRED_MODE || key_state[SDL_SCANCODE_5]) {
		// render_deferred();
	} else {
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE); //Enable depth writing, so that depth culling works.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(forward_program.handle);
		checkErrors("After glUseProgram(forward_program.handle)");
		forward_update_point_lights(&forward_program, &point_lights);
		checkErrors("After forward_update_point_lights");
		//glUniform3fv(forward_program.camera_position, 1, eye_frame.T);
		draw_forward(&forward_program, newship_buffers, ship_frame);
		draw_forward(&forward_program, room_buffers, room_frame);
		checkErrors("After regular forward draw");
		//draw_forward(&forward_program, grid_buffers, grid_frame);
		checkErrors("After updating uOrigin");
		glUseProgram(outline_program.handle);
		glUniform3fv(outline_program.uOrigin, 1, eye_frame.T);
		//draw_forward_adjacent(&outline_program, newship_buffers, ship_frame);
		draw_forward_adjacent(&outline_program, room_buffers, room_frame);
		checkErrors("After drawing outline");
		glUseProgram(shadow_program.handle);
		glUniform3fv(shadow_program.uOrigin, 1, eye_frame.T);
		glUniform3fv(shadow_program.gLightPos, 1, point_lights.position[0].A);
		draw_forward_adjacent(&shadow_program, newship_buffers, ship_frame);
		checkErrors("After drawing shadow lines");
		glUseProgram(skybox_program.handle);
		AMAT4 skybox_frame = {
			.a = mat3_scalemat(skybox_scale.x, skybox_scale.y, skybox_scale.z),
			.t = eye_frame.t};
		//draw_skybox_forward(&skybox_program, cube_buffers, skybox_frame);
		//draw_stars();
		checkErrors("After forward junk");
	}
}

void update(float dt)
{
	func_list_call(&update_func_list);
	static float light_time = 0;
	static AMAT4 velocity = AMAT4_IDENT;
	static AMAT4 ship_cam = {.a = MAT3_IDENT, .T = {0, 4, 8}};
	light_time += dt * 0.2;
	point_lights.position[0] = vec3_new(10*cos(light_time), 4, 10*sin(light_time)-8);

	float ts = 1/300000.0;
	float rs = 1/600000.0;

	//If you're moving forward, turn the light on to show it.
	//point_lights.enabled_for_draw[2] = (axes[LEFTY] < 0)?true:false;
	point_lights.enabled_for_draw[2] = key_state[SDL_SCANCODE_6];
	float camera_speed = 20;
	float ship_speed = 1;
	// if (key_state[SDL_SCANCODE_2])
	// 	draw_light_bounds = true;
	// else
	// 	draw_light_bounds = false;
	// point_lights.position[3] = vec3_add(point_lights.position[3], (VEC3){{
	// 	(key_state[SDL_SCANCODE_RIGHT] - key_state[SDL_SCANCODE_LEFT]) * dt * camera_speed,
	// 	0,
	// 	(key_state[SDL_SCANCODE_DOWN] - key_state[SDL_SCANCODE_UP]) * dt * camera_speed}});
	//Translate the camera using WASD.
	ship_cam.t = vec3_add(ship_cam.t,
		mat3_multvec(eye_frame.a, (VEC3){{
		(key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A]) * dt * camera_speed,
		0,
		(key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W]) * dt * camera_speed}}));

	//grid_frame.t = point_lights.position[3];
	//Set the ship's acceleration using the controller axes.
	VEC3 acceleration = mat3_multvec(ship_frame.a, (VEC3){{
		axes[LEFTX] * ts,
		0,
		axes[LEFTY] * ts}});
	//Set the ship's acceleration using the arrow keys.
	acceleration = mat3_multvec(ship_frame.a, (VEC3){{
		(key_state[SDL_SCANCODE_RIGHT] - key_state[SDL_SCANCODE_LEFT]) * dt * ship_speed,
		0,
		(key_state[SDL_SCANCODE_DOWN] - key_state[SDL_SCANCODE_UP]) * dt * ship_speed}});
	//Angular velocity is currently determined by how much each axis is deflected.
	velocity.a = mat3_rot(mat3_rotmat(0, 0, 1, -axes[RIGHTX]*rs), 1, 0, 0, axes[RIGHTY]*rs);
	//Add our acceleration to our velocity to change our speed.
	//velocity.t = vec3_add(vec3_scale(velocity.t, dt), acceleration);
	//Linear motion just sets the velocity directly.
	//velocity.t = vec3_scale(acceleration, dt*ship_speed);
	velocity.t = vec3_add(velocity.t, acceleration);
	//Rotate the ship by applying the angular velocity to the angular orientation.
	ship_frame.a = mat3_mult(ship_frame.a, velocity.a);
	//Move the ship by applying the velocity to the position.
	ship_frame.t = vec3_add(ship_frame.t, velocity.t);
	//The eye should be positioned 2 up and 8 back relative to the ship's frame.
	eye_frame.t = amat4_multpoint(ship_frame, ship_cam.t);
	//The eye should look from itself to a point in front of the ship, and its "up" should be "up" from the ship's orientation.
	eye_frame.a = mat3_lookat(
		eye_frame.t,
		amat4_multpoint(ship_frame, (VEC3){{0, 0, -4}}),
		mat3_multvec(ship_frame.a, (VEC3){{0, 1, 0}}));
	//Move the engine light to just behind the ship.
	point_lights.position[2] = amat4_multpoint(ship_frame, (VEC3){{0, 0, 6}});
}
