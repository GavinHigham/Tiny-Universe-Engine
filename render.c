#include <math.h>
#include <assert.h>
#include <string.h>
#include <GL/glew.h>
#include "render.h"
#include "shaders.h"
#include "keyboard.h"
#include "default_settings.h"
#include "affine_matrix4.h"
#include "matrix3.h"
#include "vector3.h"
#include "buffer_group.h"
#include "models.h"
#include "controller.h"
#include "deferred_framebuffer.h"
#include "lights.h"
#include "macros.h"
#include "func_list.h"
#include "shader_utils.h"
#include "gl_utils.h"

#define FOV M_PI/2.0
#define PRIMITIVE_RESTART_INDEX 0xFFFF

#define DEFERRED_MODE TRUE

static struct counted_func update_funcs_storage[10];
struct func_list update_func_list = {
	update_funcs_storage,
	0,
	LENGTH(update_funcs_storage)
};

static struct buffer_group newship_buffers;
static struct buffer_group ship_buffers;
static struct buffer_group ball_buffers;
static struct buffer_group thrust_flare_buffers;
static struct buffer_group room_buffers;
static struct buffer_group icosphere_buffers;
static GLuint quad_vbo;
static GLuint quad_ibo;
static int draw_light_bounds = FALSE;
static AM4 eye_frame = {.a = MAT3_IDENT, .T = {0, 0, 0}};
static AM4 inv_eye_frame;
static AM4 ship_frame = {.a = MAT3_IDENT, .T = {0, 0, -8}};
static AM4 room_frame = {.a = MAT3_IDENT, .T = {0, -4, -8}};

// static struct point_light point_lights[] = {
// 	{
// 		.position = {{{0, 0, 0}}},
// 		.color    = {{{1.0, 0.4, 0.4}}},
// 		.atten_c  = 1,
// 		.atten_l  = 0,
// 		.atten_e  = 1,
// 		.intensity = 50,
// 		.enabled_for_draw = TRUE
// 	},
// 	{
// 		.position = {{{0, 0, 0}}},
// 		.color    = {{{1, 1, 1}}},
// 		.atten_c  = 10,
// 		.atten_l  = 2,
// 		.atten_e  = 1,
// 		.intensity = 100,
// 		.enabled_for_draw = TRUE
// 	},
// 	{
// 		.position = {{{0, 0, 0}}},
// 		.color    = {{{.8, .8, 1}}},
// 		.atten_c  = 10,
// 		.atten_l  = 2,
// 		.atten_e  = 1,
// 		.intensity = 50,
// 		.enabled_for_draw = TRUE
// 	},
// 	{
// 		.position = {{{10, 0, -7}}},
// 		.color    = {{{0.4, 1.0, 0.4}}},
// 		.atten_c  = 0.1,
// 		.atten_l  = 0.14,
// 		.atten_e  = .07,
// 		.intensity = 0.75,
// 		.enabled_for_draw = TRUE
// 	}
// };

struct point_light_attributes point_lights = {.num_lights = 0};

static struct deferred_framebuffer gbuffer;
static GLfloat proj_mat[16];
extern int reload_shaders_signal;
//static GLuint deferred_buffer = 0;
static void buffer_quad(GLuint *vbo, GLuint *ibo);

//Set up everything needed to start rendering frames.
void init_render()
{
	glUseProgram(0);
	glClearDepth(0.0);
	glDepthFunc(GL_GREATER);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	gbuffer = new_deferred_framebuffer(SCREEN_WIDTH, SCREEN_HEIGHT);
	ship_buffers = new_buffer_group(buffer_ship);
	newship_buffers = new_buffer_group(buffer_newship);
	ball_buffers = new_buffer_group(buffer_ball);
	thrust_flare_buffers = new_buffer_group(buffer_thrust_flare);
	icosphere_buffers = new_custom_buffer_group(buffer_icosphere, 0);
	room_buffers = new_buffer_group(buffer_room);
	buffer_quad(&quad_vbo, &quad_ibo);

	make_projection_matrix(FOV, (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, -1, -1000, proj_mat, LENGTH(proj_mat));
	//Setup unchanging deferred uniforms.
	glUseProgram(deferred_program.handle);
	glUniformMatrix4fv(deferred_program.projection_matrix, 1, GL_TRUE, proj_mat);
	//Setup unchanging lighting uniforms.
	glUseProgram(point_light_program.handle);
	glUniformMatrix4fv(point_light_program.projection_matrix, 1, GL_TRUE, proj_mat);
	glUniform1i(point_light_program.gPositionMap, GBUFFER_TEXTURE_TYPE_POSITION);
	glUniform1i(point_light_program.gColorMap, GBUFFER_TEXTURE_TYPE_DIFFUSE);
	glUniform1i(point_light_program.gNormalMap, GBUFFER_TEXTURE_TYPE_NORMAL);
	glUniform2f(point_light_program.gScreenSize, SCREEN_WIDTH, SCREEN_HEIGHT);
	glUseProgram(point_light_wireframe_program.handle);
	glUniformMatrix4fv(point_light_wireframe_program.projection_matrix, 1, GL_TRUE, proj_mat);

	//Position, color, atten_c, atten_l, atten_e, intensity
	new_point_light(&point_lights, (V3){{{0, 0, 0}}},   (V3){{{1.0, 0.4, 0.4}}},   1,    0,    1, 50);
	new_point_light(&point_lights, (V3){{{0, 0, 0}}},   (V3){{{1.0, 1.0, 1.0}}},  10,    2,    1, 100);
	new_point_light(&point_lights, (V3){{{0, 0, 0}}},   (V3){{{0.8, 0.8, 1.0}}},  10,    2,    1, 50);
	new_point_light(&point_lights, (V3){{{10, 0, -7}}}, (V3){{{0.4, 1.0, 0.4}}}, 0.1, 0.14, 0.07, 0.75);

	for (int i = 0; i < point_lights.num_lights; i++) {
		printf("Light %i radius is %f\n", i, point_lights.radius[i]);
	}

	glUseProgram(0);
	checkErrors("After init");
}

void deinit_render()
{
	delete_deferred_framebuffer(gbuffer);
	delete_buffer_group(newship_buffers);
	delete_buffer_group(ball_buffers);
	delete_buffer_group(thrust_flare_buffers);
	delete_buffer_group(icosphere_buffers);
	delete_buffer_group(room_buffers);
	glDeleteBuffers(1, &quad_vbo);
	glDeleteBuffers(1, &quad_ibo);
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

static void setup_attrib_for_draw(GLuint attr_handle, GLuint buffer, GLenum attr_type, int attr_size)
{
	glEnableVertexAttribArray(attr_handle);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glVertexAttribPointer(attr_handle, attr_size, attr_type, GL_FALSE, 0, NULL);
}

static void draw_vcol_vnorm(struct shader_prog *program, struct buffer_group bg, AM4 model_matrix)
{	
	GLfloat mvm_buf[16];
	checkErrors("Before setting uniforms.");
	AM4 model_view_matrix = AM4_mult(inv_eye_frame, model_matrix);
	AM4_to_array(mvm_buf, LENGTH(mvm_buf), model_view_matrix);
	glUniformMatrix4fv(program->MVM, 1, GL_TRUE, mvm_buf);
	mat3_v3_to_array(mvm_buf, LENGTH(mvm_buf), mat3_transp(model_view_matrix.a), (V3){{{0, 0, 0}}});
	glUniformMatrix4fv(program->NMVM, 1, GL_TRUE, mvm_buf);
	checkErrors("Before glDrawElements");
	setup_attrib_for_draw(program->vPos, bg.vbo, GL_FLOAT, 3);
	setup_attrib_for_draw(program->vColor, bg.cbo, GL_FLOAT, 3);
	setup_attrib_for_draw(program->vNormal, bg.nbo, GL_FLOAT, 3);
	//indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);
	glDrawElements(GL_TRIANGLES, bg.index_count, GL_UNSIGNED_INT, NULL);
	checkErrors("After glDrawElements");
	//Clean up?
	glDisableVertexAttribArray(program->vPos);
	glDisableVertexAttribArray(program->vColor);
	glDisableVertexAttribArray(program->vNormal);
}

static void buffer_quad(GLuint *vbo, GLuint *ibo)
{
	glGenBuffers(1, vbo);
	glGenBuffers(1, ibo);
	GLfloat positions[] = {
		-1, -1, 0,
		 1, -1, 0,
		 1,  1, 0,
		-1,  1, 0
	};
	GLuint indices[] = {
		0, 1, 2, 3
	};

	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

static void draw_light(struct shader_prog *program, struct point_light_attributes *lights, int i)
{
	//Buffer the MVM to the shader.
	GLfloat mvm_buf[16];
	float radius = lights->radius[i];
	V3 position = lights->position[i];
	V3 color = lights->color[i];
	AM4 model_matrix = {.a = mat3_scalemat(radius, radius, radius), .t = position};
	AM4 model_view_matrix = AM4_mult(inv_eye_frame, model_matrix);
	AM4_to_array(mvm_buf, LENGTH(mvm_buf), model_view_matrix);
	glUniformMatrix4fv(program->MVM, 1, GL_TRUE, mvm_buf);

	// AM4 model_view_matrix = AM4_IDENT;
	// AM4_to_array(mvm_buf, LENGTH(mvm_buf), model_view_matrix);
	// glUniformMatrix4fv(program->MVM, 1, GL_TRUE, mvm_buf);

	V3 light_position_camera_space = AM4_multpoint(inv_eye_frame, position);
	glUniform3f(program->uLight_pos, light_position_camera_space.x, light_position_camera_space.y, light_position_camera_space.z);
	glUniform3f(program->uLight_col, color.r, color.g, color.b);
	glUniform4f(program->uLight_attr, lights->atten_c[i], lights->atten_l[i], lights->atten_e[i], lights->intensity[i]);

	// setup_attrib_for_draw(program->vPos, quad_vbo, GL_FLOAT, 3);
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ibo);
	setup_attrib_for_draw(program->vPos, icosphere_buffers.vbo, GL_FLOAT, 3);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, icosphere_buffers.ibo);
	glDrawElements(GL_TRIANGLES, icosphere_buffers.index_count, GL_UNSIGNED_INT, NULL);
	//Clean up?
	glDisableVertexAttribArray(program->vPos);
}

static void draw_light_wireframe(struct shader_prog *program, struct point_light_attributes *lights, int i)
{
	//Buffer the MVM to the shader.
	GLfloat mvm_buf[16];
	float radius = lights->radius[i];
	V3 color = lights->color[i];
	AM4 model_matrix = {.a = mat3_scalemat(radius, radius, radius), .t = lights->position[i]};
	AM4 model_view_matrix = AM4_mult(inv_eye_frame, model_matrix);
	AM4_to_array(mvm_buf, LENGTH(mvm_buf), model_view_matrix);
	glUniformMatrix4fv(program->MVM, 1, GL_TRUE, mvm_buf);
	glUniform3f(program->uLight_col, color.r, color.g, color.b);

	setup_attrib_for_draw(program->vPos, icosphere_buffers.vbo, GL_FLOAT, 3);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, icosphere_buffers.ibo);
	glDrawElements(GL_TRIANGLES, icosphere_buffers.index_count, GL_UNSIGNED_INT, NULL);
	//Clean up?
	glDisableVertexAttribArray(program->vPos);
}

static void render_to_deferred(struct shader_prog *program)
{
	glUseProgram(program->handle);

	draw_vcol_vnorm(program, newship_buffers, ship_frame);
	if (axes[LEFTY] < 0) {
		draw_vcol_vnorm(program, thrust_flare_buffers, ship_frame);
	}
	draw_vcol_vnorm(program, room_buffers, room_frame);
}

//Draw the deferred framebuffer textures to the screen, for debugging.
static void blit_deferred()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.0f, 0.0f, 0.25f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer.fbo);

	GLsizei HalfWidth = (GLsizei)(SCREEN_WIDTH / 2.0f);
	GLsizei HalfHeight = (GLsizei)(SCREEN_HEIGHT / 2.0f);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_POSITION);
	glBlitFramebuffer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, HalfWidth, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_DIFFUSE);
	glBlitFramebuffer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, HalfHeight, HalfWidth, SCREEN_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_NORMAL);
	glBlitFramebuffer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, HalfWidth, HalfHeight, SCREEN_WIDTH, SCREEN_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_TEXTURE_TYPE_TEXCOORD);
	// glBlitFramebuffer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, HalfWidth, 0, SCREEN_WIDTH, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	checkErrors("After rendering from deferred."); 
}

static void draw_point_lights(struct point_light_attributes *lights)
{
	int num_lights = lights->num_lights;
	glUseProgram(point_light_program.handle);
	for (int i = 0; i < num_lights; i++)
		if (lights->enabled_for_draw[i])
			draw_light(&point_light_program, lights, i);

	if (draw_light_bounds) {
		glDisable(GL_BLEND);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUseProgram(point_light_wireframe_program.handle);
		for (int i = 0; i < num_lights; i++)
			if (lights->enabled_for_draw[i])
				draw_light_wireframe(&point_light_wireframe_program, lights, i);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_BLEND);
	}
}

void render()
{
	inv_eye_frame = AM4_inverse(eye_frame); //Only need to do this once per frame.
	//Begin the deferred pass setup.
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gbuffer.fbo);
	glDepthMask(GL_TRUE); //Enable depth writing, so that depth culling works.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST); //Depth testing.
	glEnable(GL_CULL_FACE); //Backface culling.
	glDisable(GL_BLEND); //No blending, or we'll get weird artifacts at discontinuities.
	//Render vertex attributes to the gbuffer.
	render_to_deferred(&deferred_program);
	//Begin the lighting pass.
	bind_deferred_for_reading(gbuffer);
	glDepthMask(GL_FALSE); //We don't want to write to the depth buffer.
	glDisable(GL_DEPTH_TEST); //Lights can occupy the same volume, so we don't want depth testing.
	glEnable(GL_BLEND); //We're going to blend the contribution from each individual light.
	glBlendEquation(GL_FUNC_ADD); //The light contributions get blended additively.
	glBlendFunc(GL_ONE, GL_ONE); //Just straight addition.
	glDisable(GL_CULL_FACE); //We'll need the backfaces for some fancy stencil buffer tricks.
	glClear(GL_COLOR_BUFFER_BIT); //Clear the previous scene.
	draw_point_lights(&point_lights);
	//blit_deferred(); //Use this to overlay the deferred buffers, for debugging.
	glUseProgram(0);
}

void update(float dt)
{
	func_list_call(&update_func_list);
	static float light_time = 0;
	static AM4 velocity = AM4_IDENT;
	light_time += dt;
	point_lights.position[0] = v3_new(10*cos(light_time), 0, 10*sin(light_time)-8);

	float ts = 1/30000000.0;
	float rs = 1/600000.0;

	//If you're moving forward, turn the light on to show it.
	point_lights.enabled_for_draw[2] = (axes[LEFTY] < 0)?TRUE:FALSE;
	float camera_speed = 20;
	float ship_speed = 12000;
	if (key_state[SDL_SCANCODE_2])
		draw_light_bounds = TRUE;
	else
		draw_light_bounds = FALSE;
	//Translate the camera using the arrow keys.
	point_lights.position[3] = v3_add(point_lights.position[3], (V3){{{
		(key_state[SDL_SCANCODE_RIGHT] - key_state[SDL_SCANCODE_LEFT]) * dt * camera_speed,
		0,
		(key_state[SDL_SCANCODE_DOWN] - key_state[SDL_SCANCODE_UP]) * dt * camera_speed}}});
	//Set the ship's acceleration using the controller axes.
	V3 acceleration = mat3_multvec(ship_frame.a, (V3){{{
		axes[LEFTX] * ts,
		0,
		axes[LEFTY] * ts}}});
	//Angular velocity is currently determined by how much each axis is deflected.
	velocity.a = mat3_rot(mat3_rotmat(0, 0, 1, -axes[RIGHTX]*rs), 1, 0, 0, axes[RIGHTY]*rs);
	//Add our acceleration to our velocity to change our speed.
	//velocity.t = v3_add(v3_scale(velocity.t, dt), acceleration);
	//Linear motion just sets the velocity directly.
	velocity.t = v3_scale(acceleration, dt*ship_speed);
	//Rotate the ship by applying the angular velocity to the angular orientation.
	ship_frame.a = mat3_mult(ship_frame.a, velocity.a);
	//Move the ship by applying the velocity to the position.
	ship_frame.t = v3_add(ship_frame.t, velocity.t);
	//The eye should be positioned 2 up and 8 back relative to the ship's frame.
	eye_frame.t = AM4_multpoint(ship_frame, (V3){{{0, 2, 8}}});
	//The eye should look from itself to a point in front of the ship, and its "up" should be "up" from the ship's orientation.
	eye_frame.a = mat3_lookat(
		eye_frame.t,
		AM4_multpoint(ship_frame, (V3){{{0, 0, -4}}}),
		mat3_multvec(ship_frame.a, (V3){{{0, 1, 0}}}));
	//Move the engine light to just behind the ship.
	point_lights.position[2] = AM4_multpoint(ship_frame, (V3){{{0, 0, 6}}});
}
