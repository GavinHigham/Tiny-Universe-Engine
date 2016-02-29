#include <GL/glew.h>
#include "render_deferred.h"
#include "shaders/shaders.h"
#include "lights.h"
#include "macros.h"
#include "buffer_group.h"
#include "shader_utils.h"
#include "default_settings.h"
#include "math/affine_matrix4.h"
#include "deferred_framebuffer.h"
#include "keyboard.h"

extern GLuint gVAO;
extern AMAT4 inv_eye_frame;
extern struct buffer_group newship_buffers;
extern struct buffer_group ship_buffers;
extern struct buffer_group ball_buffers;
extern struct buffer_group thrust_flare_buffers;
extern struct buffer_group room_buffers;
extern struct buffer_group icosphere_buffers;
extern struct buffer_group big_asteroid_buffers;
extern struct buffer_group grid_buffers;
extern struct buffer_group cube_buffers;
extern AMAT4 eye_frame;
extern AMAT4 inv_eye_frame;
extern AMAT4 ship_frame;
extern AMAT4 room_frame;
extern AMAT4 grid_frame; 
extern AMAT4 big_asteroid_frame;
extern GLuint quad_vbo;
extern GLuint quad_ibo;
static struct deferred_framebuffer gbuffer;
static struct accumulation_buffer lbuffer;
extern GLfloat proj_mat[16];
bool draw_light_bounds = false;
extern struct point_light_attributes point_lights;
extern void setup_attrib_for_draw(GLuint attr_handle, GLuint buffer, GLenum attr_type, int attr_size);

void init_deferred_render()
{
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
	gbuffer = new_deferred_framebuffer(SCREEN_WIDTH, SCREEN_HEIGHT);
	lbuffer = new_accumulation_buffer(SCREEN_WIDTH, SCREEN_HEIGHT);
}

void deinit_deferred_render()
{
	delete_deferred_framebuffer(gbuffer);
	delete_accumulation_buffer(lbuffer);
}

void buffer_quad(GLuint *vbo, GLuint *ibo)
{
	glBindVertexArray(gVAO);
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

static void draw_vcol_vnorm(struct shader_prog *program, struct buffer_group bg, AMAT4 model_matrix)
{	
	glBindVertexArray(bg.vao);
	GLfloat mvm_buf[16];
	checkErrors("Before setting uniforms");
	AMAT4 model_view_matrix = amat4_mult(inv_eye_frame, model_matrix);
	//Send model_view_matrix.
	amat4_to_array(mvm_buf, LENGTH(mvm_buf), model_view_matrix);
	glUniformMatrix4fv(program->model_view_matrix, 1, GL_TRUE, mvm_buf);
	//Send model_matrix
	amat4_to_array(mvm_buf, LENGTH(mvm_buf), model_matrix);
	glUniformMatrix4fv(program->model_matrix, 1, GL_TRUE, mvm_buf);
	//Send transpmodel_matrix.
	mat3_vec3_to_array(mvm_buf, LENGTH(mvm_buf), mat3_transp(model_matrix.a), (VEC3){{0, 0, 0}});
	glUniformMatrix4fv(program->transp_model_matrix, 1, GL_TRUE, mvm_buf);
	//Draw!
	glDrawElements(bg.primitive_type, bg.index_count, GL_UNSIGNED_INT, NULL);
}

void draw_light(struct shader_prog *program, struct point_light_attributes *lights, int i)
{
	glBindVertexArray(gVAO);
	//Buffer the MVM to the shader.
	GLfloat mvm_buf[16];
	float radius = lights->radius[i];
	VEC3 position = lights->position[i];
	VEC3 color = lights->color[i];
	AMAT4 model_matrix = {.a = mat3_scalemat(radius, radius, radius), .t = position};
	AMAT4 model_view_matrix = amat4_mult(inv_eye_frame, model_matrix);
	amat4_to_array(mvm_buf, LENGTH(mvm_buf), model_view_matrix);
	glUniformMatrix4fv(program->MVM, 1, GL_TRUE, mvm_buf);

	// AMAT4 model_view_matrix = AMAT4_IDENT;
	// amat4_to_array(mvm_buf, LENGTH(mvm_buf), model_view_matrix);
	// glUniformMatrix4fv(program->MVM, 1, GL_TRUE, mvm_buf);

	glUniform3fv(program->uLight_pos, 1, position.A);
	glUniform3fv(program->uLight_col, 1, color.A);
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
	glBindVertexArray(gVAO);
	//Buffer the MVM to the shader.
	GLfloat mvm_buf[16];
	float radius = lights->radius[i];
	VEC3 color = lights->color[i];
	AMAT4 model_matrix = {.a = mat3_scalemat(radius, radius, radius), .t = lights->position[i]};
	AMAT4 model_view_matrix = amat4_mult(inv_eye_frame, model_matrix);
	amat4_to_array(mvm_buf, LENGTH(mvm_buf), model_view_matrix);
	glUniformMatrix4fv(program->MVM, 1, GL_TRUE, mvm_buf);
	glUniform3fv(program->uLight_col, 1, color.A);

	setup_attrib_for_draw(program->vPos, icosphere_buffers.vbo, GL_FLOAT, 3);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, icosphere_buffers.ibo);
	glDrawElements(GL_TRIANGLES, icosphere_buffers.index_count, GL_UNSIGNED_INT, NULL);
	//Clean up?
	glDisableVertexAttribArray(program->vPos);
}

static void render_to_deferred(struct shader_prog *program)
{
	glUseProgram(program->handle);
	//draw_vcol_vnorm(program, big_asteroid_buffers, big_asteroid_frame);
	draw_vcol_vnorm(program, newship_buffers, ship_frame);
	// if (axes[LEFTY] < 0) {
	// 	draw_vcol_vnorm(program, thrust_flare_buffers, ship_frame);
	// }
	draw_vcol_vnorm(program, room_buffers, room_frame);
	//draw_forward(program, grid_buffers, grid_frame);
}

static void render_effects_pass(struct shader_prog *program)
{
	glUseProgram(program->handle);
	glClear(GL_COLOR_BUFFER_BIT); //Clear the previous scene.
	setup_attrib_for_draw(program->vPos, quad_vbo, GL_FLOAT, 3);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ibo);
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);
}

//Draw the deferred framebuffer textures to the screen, for debugging.
static void blit_deferred()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glClearColor(0.0f, 0.0f, 0.25f, 1.0f);
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

	glBindFramebuffer(GL_READ_FRAMEBUFFER, lbuffer.fbo);
	if (key_state[SDL_SCANCODE_4])
		glReadBuffer(GL_COLOR_ATTACHMENT1);
	else
		glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBlitFramebuffer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, HalfWidth, 0, SCREEN_WIDTH, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	checkErrors("After rendering from deferred."); 
}

static void draw_point_lights(struct point_light_attributes *lights)
{
	int num_lights = lights->num_lights;
	glUseProgram(point_light_program.handle);
	glUniform3fv(point_light_program.camera_position, 1, eye_frame.A);
	for (int i = 0; i < num_lights; i++)
		if (lights->enabled_for_draw[i])
			draw_light(&point_light_program, lights, i);

	if (draw_light_bounds) {
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUseProgram(point_light_wireframe_program.handle);
		for (int i = 0; i < num_lights; i++)
			if (lights->enabled_for_draw[i])
				draw_light_wireframe(&point_light_wireframe_program, lights, i);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
	}
}

void render_deferred()
{
	//Begin the deferred pass setup.
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gbuffer.fbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND); //No blending, or we'll get weird artifacts at discontinuities.
	//Render vertex attributes to the gbuffer.
	render_to_deferred(&deferred_program);
	//Begin the lighting pass.
	bind_deferred_for_reading(gbuffer, lbuffer);
	glBlitFramebuffer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glDepthMask(GL_FALSE); //We don't want to write to the depth buffer.
	glDisable(GL_DEPTH_TEST); //Lights can occupy the same volume, so we don't want depth testing.
	glEnable(GL_BLEND); //We're going to blend the contribution from each individual light.
	glBlendEquation(GL_FUNC_ADD); //The light contributions get blended additively.
	glBlendFunc(GL_ONE, GL_ONE); //Just straight addition.
	//glDisable(GL_CULL_FACE); //We'll need the backfaces for some fancy stencil buffer tricks.
	glCullFace(GL_FRONT); //For now, we'll just cull front faces to avoid double brightness.
	glClear(GL_COLOR_BUFFER_BIT); //Clear the previous scene.
	draw_point_lights(&point_lights);
	//Effects pass / sky pass / gamma correction / bloom will go here.
	bind_accumulation_for_reading(lbuffer);
	//glEnable(GL_FOG);
	glCullFace(GL_BACK);
	render_effects_pass(&effects_program);
	//glDisable(GL_FOG);
	if (key_state[SDL_SCANCODE_3])
		blit_deferred(); //Use this to overlay the deferred buffers, for debugging.
	glUseProgram(0);
	glEnable(GL_DEPTH_TEST); //Depth testing.
	//glEnable(GL_CULL_FACE); //Backface culling.
	glDepthMask(GL_TRUE); //Enable depth writing, so that depth culling works.
}