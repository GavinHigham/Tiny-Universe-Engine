#include <math.h>
#include <assert.h>
#include <string.h>
#include <GL/glew.h>
#include "render.h"
#include "init.h"
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

#define FOV M_PI/2.0
#define PRIMITIVE_RESTART_INDEX 0xFFFF

#define DEFERRED_MODE FALSE


static struct buffer_group newship_buffers;
static struct buffer_group ball_buffers;
static struct buffer_group thrust_flare_buffers;
static struct buffer_group thrust_flare_forward_buffers;
static AM4 eye_frame = {.a = MAT3_IDENT, .T = {0, 0, 0}};;
static AM4 ship_frame = {.a = MAT3_IDENT, .T = {0, 0, -8}};
static V3 light_pos_vec = {{{0, 0, 0}}};
static int thrusting = 0;
static struct deferred_framebuffer gbuffer;
//static GLuint deferred_buffer = 0;

//Set up everything needed to start rendering frames.
void init_render()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearDepth(0.0);
	glDepthFunc(GL_GREATER);
	glClearColor(0.0f, 0.0f, 0.25f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	checkErrors("Before creating framebuffer");
	gbuffer = new_deferred_framebuffer(SCREEN_WIDTH, SCREEN_HEIGHT);
	checkErrors("After creating framebuffer");
	newship_buffers = new_buffer_group(buffer_newship);
	ball_buffers = new_buffer_group(buffer_ball);
	thrust_flare_buffers = new_buffer_group(buffer_thrust_flare);
	thrust_flare_forward_buffers = new_buffer_group(buffer_thrust_flare_forward);
	checkErrors("Before setting program");
	glUseProgram(deferred_program.handle);
	GLfloat proj_mat[16];
	make_projection_matrix(FOV, (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, -1, -1000, proj_mat, sizeof(proj_mat)/sizeof(proj_mat[0]));
	checkErrors("Before sending projection matrix");
	glUniformMatrix4fv(deferred_program.projection_matrix, 1, GL_TRUE, proj_mat);
	checkErrors("After sending projection matrix");
	glUseProgram(0);
	checkErrors("After init");
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
	assert(buf_len == sizeof(tmp)/sizeof(GLfloat));
	if (a >= 0)
		tmp[0] /= a;
	else
		tmp[5] *= a;
	memcpy(buf, tmp, sizeof(tmp));
}

void draw(struct shader_prog *program, struct buffer_group bg, AM4 model_view_matrix)
{	
	GLfloat mvm_buf[16];
	checkErrors("Before setting uniforms.");
	model_view_matrix = AM4_mult(AM4_inverse(eye_frame), model_view_matrix);
	AM4_to_array(mvm_buf, sizeof(mvm_buf)/sizeof(mvm_buf[0]), model_view_matrix);
	glUniformMatrix4fv(program->MVM, 1, GL_TRUE, mvm_buf);
	mat3_v3_to_array(mvm_buf, sizeof(mvm_buf)/sizeof(mvm_buf[0]), mat3_transp(model_view_matrix.a), (V3){{{0, 0, 0}}});
	glUniformMatrix4fv(program->NMVM, 1, GL_TRUE, mvm_buf);
	glUniform3f(program->light_pos, light_pos_vec.x, light_pos_vec.y, light_pos_vec.z);
	checkErrors("Before glDrawElements");
	//vertex_pos
	glEnableVertexAttribArray(program->vPos);
	glBindBuffer(GL_ARRAY_BUFFER, bg.vbo);
	glVertexAttribPointer(program->vPos, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	checkErrors("After enabling vPos");
	//vColor
	glEnableVertexAttribArray(program->vColor);
	glBindBuffer(GL_ARRAY_BUFFER, bg.cbo);
	glVertexAttribPointer(program->vColor, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	checkErrors("After enabling vColor");
	//vNormal
	glEnableVertexAttribArray(program->vNormal);
	glBindBuffer(GL_ARRAY_BUFFER, bg.nbo);
	glVertexAttribPointer(program->vNormal, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	checkErrors("After enabling vNormal");
	//indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);
	glDrawElements(GL_TRIANGLES, bg.index_count, GL_UNSIGNED_INT, NULL);
	checkErrors("After glDrawElements");
	//Clean up?
	glDisableVertexAttribArray(program->vPos);
	glDisableVertexAttribArray(program->vColor);
	glDisableVertexAttribArray(program->vNormal);
}

void draw_deferred(struct shader_prog *program, struct buffer_group bg, AM4 model_view_matrix)
{
	GLfloat mvm_buf[16];
	checkErrors("Before setting uniforms.");
	model_view_matrix = AM4_mult(AM4_inverse(eye_frame), model_view_matrix);
	AM4_to_array(mvm_buf, sizeof(mvm_buf)/sizeof(mvm_buf[0]), model_view_matrix);
	glUniformMatrix4fv(program->MVM, 1, GL_TRUE, mvm_buf);
	mat3_v3_to_array(mvm_buf, sizeof(mvm_buf)/sizeof(mvm_buf[0]), mat3_transp(model_view_matrix.a), (V3){{{0, 0, 0}}});
	glUniformMatrix4fv(program->NMVM, 1, GL_TRUE, mvm_buf);
	checkErrors("Before glDrawElements");
	//vertex_pos
	glEnableVertexAttribArray(program->vPos);
	glBindBuffer(GL_ARRAY_BUFFER, bg.vbo);
	glVertexAttribPointer(program->vPos, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	checkErrors("After enabling vPos");
	//indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);
	glDrawElements(GL_TRIANGLES, bg.index_count, GL_UNSIGNED_INT, NULL);
	checkErrors("After glDrawElements");
	//Clean up?
	glDisableVertexAttribArray(program->vPos);
}

void render_to_deferred(struct shader_prog *program)
{
	checkErrors("Before rendering to deferred");
	glUseProgram(program->handle);
	glDepthMask(GL_TRUE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	checkErrors("Before drawing.");
	//glUniform1f(program->alpha, 1.0);
	//glUniform1f(program->emit, 0.0);
	draw(program, newship_buffers, ship_frame);
	//glUniform1f(program->emit, 1.0);
	draw(program, ball_buffers, (AM4){.a = MAT3_IDENT, .t = light_pos_vec});
	if (thrusting == 1) {
		//glUniform1f(program->alpha, 0.8);
		draw(program, thrust_flare_buffers, ship_frame);
	} else if (thrusting == 2) {
		//glUniform1f(program->alpha, 0.8);
		draw(program, thrust_flare_forward_buffers, ship_frame);
	}
	checkErrors("After drawing.");
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);

	glUseProgram(0);
}

void render_from_deferred()
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

void render()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gbuffer.fbo);
	render_to_deferred(&deferred_program);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer.fbo);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(lighting_program.handle);
	draw_deferred(&lighting_program, ball_buffers, (AM4)AM4_IDENT);
	//render_from_deferred();
}

void update(float dt)
{
	static float light_time = 0;
	static AM4 velocity = AM4_IDENT;
	light_time += dt;
	light_pos_vec = v3_new(0, 0, 0);//v3_new(10*cos(light_time), 0, 10*sin(light_time)-8);

	float ts = 1/30000000.0;
	float rs = 1/600000.0;
	if (axes[LEFTY] < 0)
		thrusting = 1;
	else if (axes[LEFTY] > 0)
		thrusting = 2;
	else
		thrusting = 0;
	velocity.a = mat3_rot(mat3_rotmat(0, 0, 1, -axes[RIGHTX]*rs), 1, 0, 0, axes[RIGHTY]*rs);
	velocity.t = v3_add(velocity.t, mat3_multvec(ship_frame.a, (V3){{{axes[LEFTX]*ts, 0, axes[LEFTY]*ts}}}));
	ship_frame.a = mat3_mult(ship_frame.a, velocity.a);
	ship_frame.t = v3_add(ship_frame.t, velocity.t);
	//eye_frame.a = mat3_lookat(eye_frame.t, ship_frame.t, (V3){{{0, 1, 0}}});
}
