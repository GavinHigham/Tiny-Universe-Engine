#include "twotri_scene.h"
#include "scene.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "macros.h"
#include "math/utility.h"
#include "drawf.h"
#include "input_event.h"
//#include "space/triangular_terrain_tile.h"
#include "configuration/lua_configuration.h"
#include "trackball/trackball.h"
#include "meter/meter.h"
#include "deferred_framebuffer.h"

#include <glla.h>
#include <GL/glew.h>
#include <stdio.h>
#include <assert.h>
//#include <SDL2_image/SDL_image.h>

/* Implementing scene "interface" */

SCENE_IMPLEMENT(twotri);

static float screen_width = 640, screen_height = 480;
static int mouse_x = 0, mouse_y = 0;
static struct trackball twotri_trackball;
static struct color_buffer cbuffer;
static bool clear_accum = false;
static int accum_frames = 1;

/* Lua Config */
extern lua_State *L;

/* OpenGL Variables */

static GLint POS_ATTR = 1;
static GLuint SHADER, VAO, VBO, RESOLUTION_UNIF, MOUSE_UNIF, TIME_UNIF, FOCAL_UNIF, DIR_UNIF, EYE_UNIF, BRIGHT_UNIF, ROTATION_UNIF;
static GLuint DSHADER, DRESOLUTION_UNIF, DAB_UNIF, DNFRAMES_UNIF;
GLfloat vertices[] = {-1, -1, 1, -1, -1, 1, 1, 1};
float g_brightness;
float g_rotation;

int twotri_scene_init()
{
	float FOV = M_PI/2;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	/* Shader initialization */
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("glsl330", "#version 330");

	char *vsh_key = getglobstr(L, "twotri_vsh_key", "");
	char *fsh_key = getglobstr(L, "twotri_fsh_key", "");

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "versions.glsl330", vsh_key),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "versions.glsl330", "common.noise.GL33", fsh_key),
	}, shader_deferred[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "versions.glsl330", vsh_key),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "versions.glsl330", "spiral.deferred.GL33"),
	};
	SHADER = glsw_new_shader_program(shader, LENGTH(shader));
	DSHADER = glsw_new_shader_program(shader_deferred, LENGTH(shader_deferred));
	glswShutdown();

	if (!SHADER || !DSHADER) {
		twotri_scene_deinit();
		return -1;
	}

	/* Retrieve uniform variable handles */

	RESOLUTION_UNIF  = glGetUniformLocation(SHADER, "iResolution");
	MOUSE_UNIF       = glGetUniformLocation(SHADER, "iMouse");
	TIME_UNIF        = glGetUniformLocation(SHADER, "iTime");
	FOCAL_UNIF       = glGetUniformLocation(SHADER, "iFocalLength");
	DIR_UNIF         = glGetUniformLocation(SHADER, "dir_mat");
	EYE_UNIF         = glGetUniformLocation(SHADER, "eye_pos");
	BRIGHT_UNIF      = glGetUniformLocation(SHADER, "brightness");
	ROTATION_UNIF    = glGetUniformLocation(SHADER, "rotation");
	DRESOLUTION_UNIF = glGetUniformLocation(DSHADER, "iResolution");
	DAB_UNIF         = glGetUniformLocation(DSHADER, "cbuffer");
	DNFRAMES_UNIF    = glGetUniformLocation(DSHADER, "num_frames_accum");
	checkErrors("After getting uniform handles");
	glUseProgram(SHADER);
	glUniform1f(FOCAL_UNIF, 1.0/tan(FOV/2.0));

	/* Vertex data */

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	/* Vertex attributes */

	glEnableVertexAttribArray(POS_ATTR);
	glVertexAttribPointer(POS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, false);
	checkErrors("After pos attr");

	/* Accumulation Buffer */
	cbuffer = color_buffer_new(screen_width, screen_height);

	/* Misc. OpenGL bits */

	glClearDepth(1.0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBindVertexArray(0);

	glUseProgram(0);

	/* Set up trackball */
	twotri_trackball = trackball_new((vec3){0, 0, 0}, 50);
	trackball_set_speed(&twotri_trackball, 1.0/50.0, 1.0/200.0, 1.0/10.0);
	trackball_set_bounds(&twotri_trackball, M_PI/2.0 - 0.0001, M_PI/2.0 - 0.0001, INFINITY, INFINITY);

	/* Set up meter module */
	meter_init(screen_width, screen_height, 20);
	{
		meter_add("Brightness", 100, 20, 0.0, 5.0, 100.0);
		meter_target("Brightness", &g_brightness);
		meter_position("Brightness", 5.0, 25.0);
	}
	{
		meter_add("Rotation", 150, 15, 0.0, 500.0, 1000.0);
		meter_target("Rotation", &g_rotation);
		meter_style("Rotation", (unsigned char[]){79, 79, 207, 255}, (unsigned char[]){47, 47, 95, 255}, (unsigned char[]){255, 255, 255, 255}, 2.0);
		meter_position("Rotation", 5.0, 50.0);
	}

	accum_frames = 1;

	return 0;
}

void twotri_scene_resize(float width, float height)
{
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	screen_width = width;
	screen_height = height;
	meter_resize_screen(screen_width, screen_height);
	color_buffer_delete(cbuffer);
	cbuffer = color_buffer_new(width, height);
	//make_projection_matrix(65, screen_width/screen_height, 0.1, 200, proj_mat);	
}

void twotri_scene_deinit()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(SHADER);
	meter_deinit();
}

void twotri_scene_update(float dt)
{
	// eye_frame.t += (vec3){
	// 	key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A],
	// 	key_state[SDL_SCANCODE_E] - key_state[SDL_SCANCODE_Q],
	// 	key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W],
	// } * 0.15;
	
	Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
	bool button = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
	int meter_clicked = meter_mouse(mouse_x, mouse_y, button);
	button = button && !meter_clicked;
	int scroll_x = input_mouse_wheel_sum.wheel.x, scroll_y = input_mouse_wheel_sum.wheel.y;
	if (trackball_step(&twotri_trackball, mouse_x, mouse_y, button, scroll_x, scroll_y))
		clear_accum = true;
}

void twotri_scene_render()
{
	glBindVertexArray(VAO);
	glUseProgram(SHADER);
	glClearColor(0, 0, 0, 0);
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//Draw in wireframe if 'z' is held down.
	if (key_state[SDL_SCANCODE_Z])
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	{
		float dir_mat[9];
		mat3_to_array_cm(twotri_trackball.camera.a, dir_mat);
		glUniformMatrix3fv(DIR_UNIF, 1, GL_FALSE, dir_mat);
		checkErrors("After sending dir_mat");
		glUniform3f(EYE_UNIF, VEC3_COORDS(twotri_trackball.camera.t));
	}

	// Draw to accumulation buffer.
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cbuffer.fbo);
	if (clear_accum) {
		clear_accum = false;
		accum_frames = 0;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	if (accum_frames < 180) {
		glUniform1f(ROTATION_UNIF, g_rotation);
		glUniform1f(BRIGHT_UNIF, g_brightness);
		glUniform2f(TIME_UNIF, SDL_GetTicks() / 1000.0, accum_frames);
		glUniform2f(RESOLUTION_UNIF, screen_width, screen_height);
		glUniform4f(MOUSE_UNIF, mouse_x, mouse_y, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquation(GL_FUNC_ADD);
		// glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
		glDepthMask(GL_FALSE);
		glBindFramebuffer(GL_FRAMEBUFFER, cbuffer.fbo);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		accum_frames++;
	}

	// Draw from accumulation buffer to screen.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, cbuffer.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_BLEND);
	glUseProgram(DSHADER);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cbuffer.texture);
	glUniform1i(DAB_UNIF, 0);
	glUniform1i(DNFRAMES_UNIF, accum_frames);
	glUniform2f(DRESOLUTION_UNIF, screen_width, screen_height);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_BLEND);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// glBindFramebuffer(GL_READ_FRAMEBUFFER, cbuffer.fbo);
	// glReadBuffer(GL_COLOR_ATTACHMENT0);
	// glBlitFramebuffer(0, 0, screen_width, screen_height, screen_width/2, screen_height/2, screen_width, screen_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

			
	checkErrors("After draw");
	meter_draw_all();
	checkErrors("After meter_draw_all");
	glBindVertexArray(0);
}
