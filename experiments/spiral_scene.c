#include "luaengine/lua_configuration.h"
#include "deferred_framebuffer.h"
#include "drawf.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "graphics.h"
#include "input_event.h"
#include "macros.h"
#include "math/utility.h"
#include "meter/meter.h"
#include "meter/meter_ogl_renderer.h"
#include "scene.h"
#include "shader_utils.h"
#include "space/galaxy_volume.h"
#include "spiral_scene.h"
#include "trackball/trackball.h"

#include <assert.h>
#include <glla/glla.h>
#include <math.h>
#include <stdio.h>

/* Implementing scene "interface" */

SCENE_IMPLEMENT(spiral);

static float screen_width = 640, screen_height = 480;
static int mouse_x = 0, mouse_y = 0;
static struct trackball spiral_trackball;
static struct color_buffer cbuffer;
static struct renderable_cubemap rcube;
static bool clear_active_accumulation_buffer = false;
static int texture_divisor = 0;
static int cubemap_divisor = 0;
static int max_cubemap_divisor = 180;
static int max_texture_divisor = 180;
static float iteration_bias = 0;
static bool accumulate = true;
bool show_tweaks = true;
static bool draw_to_cubemap = false;
static struct {
	float screen;
	float screen_focal;
} field_of_view;

/* Lua Config */
extern lua_State *L;

/* OpenGL Variables */

struct galaxy_ogl g_galaxy_ogl = {.attr.pos = 1};
static GLfloat vertices[] = {-1, -1, 1, -1, -1, 1, 1, 1};

struct galaxy_tweaks g_galaxy_tweaks;

/* UI */
meter_ctx g_galaxy_meters;

static void meter_clear_accum_callback(char *name, enum meter_state state, float value, void *context)
{
	clear_active_accumulation_buffer = true;
}

int spiral_scene_init()
{
	glGenVertexArrays(1, &g_galaxy_ogl.vao);
	glBindVertexArray(g_galaxy_ogl.vao);

	if (galaxy_shader_init(&g_galaxy_ogl)) {
		spiral_scene_deinit();
		return -1;
	}

	/* Vertex data */

	glGenBuffers(1, &g_galaxy_ogl.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_galaxy_ogl.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	/* Vertex attributes */

	glEnableVertexAttribArray(g_galaxy_ogl.attr.pos);
	glVertexAttribPointer(g_galaxy_ogl.attr.pos, 2, GL_FLOAT, GL_FALSE, 0, false);
	checkErrors("After pos attr");

	/* Accumulation Buffer */
	cbuffer = color_buffer_new(screen_width, screen_height);
	rcube = renderable_cubemap_new(getglob(L, "cubemap_width", 512));
	checkErrors("After renderable_cubemap_new");

	/* Misc. OpenGL bits */

	glClearDepth(1.0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBindVertexArray(0);

	glUseProgram(0);

	/* Set up trackball */
	spiral_trackball = trackball_new((vec3){0, 0, 0}, 50);
	trackball_set_speed(&spiral_trackball, 1.0/50.0, 1.0/200.0, 1.0/10.0);
	trackball_set_bounds(&spiral_trackball, M_PI/2.0 - 0.0001, M_PI/2.0 - 0.0001, INFINITY, INFINITY);

	g_galaxy_tweaks = galaxy_load_tweaks(L, "galaxy_defaults");
	galaxy_meters_init(L, &g_galaxy_meters, &g_galaxy_tweaks, screen_width, screen_height, meter_clear_accum_callback);

	field_of_view.screen = M_PI/2;
	field_of_view.screen_focal = 1.0/tan(field_of_view.screen/2.0);
	cubemap_divisor = 0;
	texture_divisor = 0;
	max_cubemap_divisor = getglob(L, "max_accum_frames", 180);
	max_texture_divisor = max_cubemap_divisor;
	accumulate = getglobbool(L, "accumulate", true);

	return 0;
}

void spiral_scene_resize(float width, float height)
{
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	screen_width = width;
	screen_height = height;
	meter_resize_screen(&g_galaxy_meters, screen_width, screen_height);
	color_buffer_delete(cbuffer);
	cbuffer = color_buffer_new(width, height);
	//make_projection_matrix(65, screen_width/screen_height, 0.1, 200, proj_mat);	
}

void spiral_scene_deinit()
{
	glDeleteVertexArrays(1, &g_galaxy_ogl.vao);
	glDeleteBuffers(1, &g_galaxy_ogl.vbo);
	color_buffer_delete(cbuffer);
	meter_deinit(&g_galaxy_meters);
}

void spiral_scene_update(float dt)
{
	// eye_frame.t += (vec3){
	// 	key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A],
	// 	key_state[SDL_SCANCODE_E] - key_state[SDL_SCANCODE_Q],
	// 	key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W],
	// } * 0.15;
	
	Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
	bool button = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
	if (show_tweaks)
		button = !meter_mouse_relative(&g_galaxy_meters, mouse_x, mouse_y, button,
		key_state[SDL_SCANCODE_LSHIFT] || key_state[SDL_SCANCODE_RSHIFT],
		key_state[SDL_SCANCODE_LCTRL]  || key_state[SDL_SCANCODE_RCTRL]) && button;
	int scroll_x = input_mouse_wheel_sum.wheel.x, scroll_y = input_mouse_wheel_sum.wheel.y;
	if (trackball_step(&spiral_trackball, mouse_x, mouse_y, button, scroll_x, scroll_y))
		clear_active_accumulation_buffer = true;

	// amat4_print(spiral_trackball.camera);
	// puts("\n");
	// for (int i = 0; i < LENGTH(g_galaxy_tweaks.tweaks1); ++i)
	// {
	// 	printf("%f ", g_galaxy_tweaks.tweaks1[i]);
	// }
	// for (int i = 0; i < LENGTH(g_galaxy_tweaks.tweaks2); ++i)
	// {
	// 	printf("%f ", g_galaxy_tweaks.tweaks2[i]);
	// }
	// for (int i = 0; i < LENGTH(g_galaxy_tweaks.bulge); ++i)
	// {
	// 	printf("%f ", g_galaxy_tweaks.bulge[i]);
	// }
	// puts("\n");

	if (key_pressed(SDL_SCANCODE_TAB))
		show_tweaks = !show_tweaks;

	if (key_pressed(SDL_SCANCODE_C))
		draw_to_cubemap = !draw_to_cubemap;

	iteration_bias = fmod(iteration_bias + 1.61803398874989, 1.0);
}

void galaxy_render_to_cubemap(struct galaxy_tweaks gt, struct galaxy_ogl gal, struct blend_params bp, amat4 camera, struct renderable_cubemap rc, int framecount, bool clear_first)
{
	glBindVertexArray(gal.vao);
	glUseProgram(gal.shader);
	glClearColor(0, 0, 0, 0);

	checkErrors("After bind gal.vao");

	//Draw in wireframe if 'z' is held down.
	if (key_state[SDL_SCANCODE_Z])
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rcube.fbo);
	checkErrors("After bind framebuffer");

	if (clear_first) {
		for (int i = 0; i < 6; i++) {
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, rcube.texture, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}
	}
	checkErrors("After clear");

	glUniform1f(gal.unif.rotation, gt.rotation);
	glUniform1f(gal.unif.diameter, gt.diameter);
	glUniform1f(gal.unif.bright, gt.brightness);
	glUniform1f(gal.unif.render_dist, gt.render_dist);
	checkErrors("After sending render_dist");
	glUniform3f(gal.unif.time, SDL_GetTicks() / 1000.0, framecount, iteration_bias);
	glUniform4f(gal.unif.mouse, mouse_x, mouse_y, 0, 0);

	glUniform4fv(gal.unif.tweaks, 1, gt.tweaks1);
	glUniform4fv(gal.unif.tweaks2, 1, gt.tweaks2);
	gt.bulge[3] = gt.bulge_width * gt.bulge_width;
	glUniform4fv(gal.unif.bulge, 1, gt.bulge);
	glUniform4fv(gal.unif.absorb, 1, gt.light_absorption);
	glUniform3f(gal.unif.eye, VEC3_COORDS(spiral_trackball.camera.t));
	{
		float dir_mat[9];
		mat3_to_array_cm(spiral_trackball.camera.a, dir_mat);
		glUniformMatrix3fv(gal.unif.dir, 1, GL_FALSE, dir_mat);
		checkErrors("After sending dir_mat");
	}
	glBindBuffer(GL_ARRAY_BUFFER, gal.vbo);
	checkErrors("After bind buffer");

	//Each accumulate step, blend additively.
	glEnable(GL_BLEND);
	glBlendFunc(bp.srcfact, bp.dstfact);
	glBlendEquation(bp.blendfn);
	glDepthMask(GL_FALSE);
	glBindFramebuffer(GL_FRAMEBUFFER, rcube.fbo);
	glViewport(0, 0, rcube.width, rcube.width);
	glUniform2f(gal.unif.resolution, rcube.width, rcube.width);
	glUniform1f(gal.unif.focal, CUBEMAP_FOCAL_LENGTH);
	checkErrors("After stuff");

	// float s90 = 1.0, c90 = 0.0, s180 = 0.0, c180 = -1.0, s270 = -1.0, c270 = 0.0;
	mat3 *c = &spiral_trackball.camera.a;
	mat3 cz180 = mat3_mult(*c, mat3_rotmatz(0.0, -1.0));
	mat3 cubemap_mats[] = {
		mat3_mult(*c, mat3_rotmaty( 1.0, 0.0)), // +X
		mat3_mult(*c, mat3_rotmaty(-1.0,  0.0)), // -X
		mat3_mult(cz180, mat3_rotmatx( 1.0,  0.0)), // +Y
		mat3_mult(cz180, mat3_rotmatx(-1.0,  0.0)), // -Y
		mat3_mult(*c, mat3_rotmaty( 0.0, -1.0)), // +Z
		*c, // -Z
	};

	for (int i = 0; i < LENGTH(cubemap_mats); i++) {
		// For each direction, draw to cubemap
		float dir_mat[9];
		mat3_to_array_cm(cubemap_mats[i], dir_mat);
		glUniformMatrix3fv(gal.unif.dir, 1, GL_FALSE, dir_mat);

		//Bind cubemap face texture and draw
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, rcube.texture, 0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	checkErrors("After draw");

	glViewport(0, 0, screen_width, screen_height);

	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
	checkErrors("After draw from cubemap");
}

void galaxy_render_from_cubemap(struct galaxy_ogl gal, struct renderable_cubemap rc, float divisor)
{
	glBindVertexArray(gal.vao);
	// Draw from accumulation buffer to screen.
	// glBindFramebuffer(GL_READ_FRAMEBUFFER, rcube.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_BLEND);
	glUseProgram(gal.dshader_cube);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, rc.texture);
	checkErrors("After bind texture");
	glUniform1i(gal.unif.dab_cube, 0);
	//The result is divided by the number of accumulated frames, averaging the passes.
	glUniform1i(gal.unif.dnframes_cube, divisor);
	// glUniform2f(gal.unif.dresolution, rcube.width, rcube.width);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

int galaxy_no_worries_just_render_cubemap(bool clear)
{
	clear = clear || clear_active_accumulation_buffer || !accumulate;
	struct blend_params bp = {
		.blendfn = GL_FUNC_ADD,
		.srcfact = GL_ONE,
		.dstfact = GL_ONE,
		// .srcfact = GL_SRC_ALPHA,
		// .dstfact = GL_ONE_MINUS_SRC_ALPHA,
		//blendcol[4] = {1.0, 1.0, 1.0, 1.0}
	};
	if (cubemap_divisor <= max_cubemap_divisor || clear) {
		galaxy_render_to_cubemap(g_galaxy_tweaks, g_galaxy_ogl, bp, spiral_trackball.camera, rcube, cubemap_divisor, clear);
		cubemap_divisor++;
		if (clear)
			cubemap_divisor = 1.0;
	}
	clear_active_accumulation_buffer = false;
	return cubemap_divisor;
}

void galaxy_bind_cubemap()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, rcube.texture);
	checkErrors("After bind texture");
}

void galaxy_render_to_texture(struct galaxy_tweaks gt, struct galaxy_ogl gal, struct blend_params bp, amat4 camera, struct color_buffer cb, int framecount, bool clear_first)
{
	glBindVertexArray(gal.vao);
	glUseProgram(gal.shader);
	glClearColor(0, 0, 0, 0);

	//Draw in wireframe if 'z' is held down.
	if (key_state[SDL_SCANCODE_Z])
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cbuffer.fbo);

	if (clear_first)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glUniform1f(gal.unif.fresh, gt.freshness);
	glUniform1i(gal.unif.samples, gt.samples);
	glUniform1f(gal.unif.rotation, gt.rotation);
	glUniform1f(gal.unif.diameter, gt.diameter);
	glUniform1f(gal.unif.bright, gt.brightness);
	glUniform1f(gal.unif.render_dist, gt.render_dist);
	checkErrors("After sending render_dist");
	glUniform3f(gal.unif.time, SDL_GetTicks() / 1000.0, framecount, iteration_bias);
	glUniform4f(gal.unif.mouse, mouse_x, mouse_y, 0, 0);

	glUniform4fv(gal.unif.tweaks, 1, gt.tweaks1);
	glUniform4fv(gal.unif.tweaks2, 1, gt.tweaks2);
	gt.bulge[3] = gt.bulge_width * gt.bulge_width;
	glUniform4fv(gal.unif.bulge, 1, gt.bulge);
	glUniform4fv(gal.unif.absorb, 1, gt.light_absorption);
	glUniform3f(gal.unif.eye, VEC3_COORDS(spiral_trackball.camera.t));
	{
		float dir_mat[9];
		mat3_to_array_cm(spiral_trackball.camera.a, dir_mat);
		glUniformMatrix3fv(gal.unif.dir, 1, GL_FALSE, dir_mat);
		checkErrors("After sending dir_mat");
	}
	glBindBuffer(GL_ARRAY_BUFFER, gal.vbo);

	// Each accumulate step, blend additively.
	glEnable(GL_BLEND);
	// glBlendFunc(GL_ONE, GL_ONE);
	glBlendFunc(bp.srcfact, bp.dstfact);

	// glBlendFunc(GL_SRC_ALPHA, GL_CONSTANT_ALPHA);
	glBlendEquation(bp.blendfn);
	glDepthMask(GL_FALSE);
	glBindFramebuffer(GL_FRAMEBUFFER, cbuffer.fbo);
	glUniform2f(gal.unif.resolution, screen_width, screen_height);
	glUniform1f(gal.unif.focal, field_of_view.screen_focal);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	checkErrors("After draw to texture");
}

void galaxy_render_from_texture(struct galaxy_ogl gal, struct color_buffer cb, float divisor)
{
	glBindVertexArray(gal.vao);
	// Draw from accumulation buffer to screen.
	glBindFramebuffer(GL_READ_FRAMEBUFFER, cbuffer.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	checkErrors("set fb");
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	checkErrors("clear");
	glDisable(GL_BLEND);
	glUseProgram(gal.dshader);
	glActiveTexture(GL_TEXTURE0);
	checkErrors("active texture");
	glBindTexture(GL_TEXTURE_2D, cbuffer.texture);
	checkErrors("bind texture");
	glUniform1i(gal.unif.dab, 0);
	//The result is divided by the number of accumulated frames, averaging the passes.
	glUniform1i(gal.unif.dnframes, divisor);
	glUniform2f(gal.unif.dresolution, screen_width, screen_height);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	checkErrors("draw");

	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
	checkErrors("After draw from texture");
}

void galaxy_demo_render(struct galaxy_tweaks gt, struct renderable_cubemap rc, struct color_buffer cb, int *cubemap_divisor, int *texture_divisor, amat4 camera)
{
	draw_to_cubemap = getglobbool(L, "cubemap_mode", false) != draw_to_cubemap;
	bool clear = clear_active_accumulation_buffer || !accumulate;
	struct blend_params bp = {
		.blendfn = GL_FUNC_ADD,
		.srcfact = GL_SRC_ALPHA,
		.dstfact = GL_ONE_MINUS_SRC_ALPHA,
		//blendcol[4] = {1.0, 1.0, 1.0, 1.0}
	};

	if (draw_to_cubemap) {
		if (*cubemap_divisor <= max_cubemap_divisor || clear) {
			galaxy_render_to_cubemap(g_galaxy_tweaks, g_galaxy_ogl, bp, spiral_trackball.camera, rc, *cubemap_divisor, false);
			// cubemap_divisor = clear ? 1 : *cubemap_divisor + 1;
			(*cubemap_divisor)++;
			checkErrors("After draw to cubemap");
		}
		// galaxy_render_from_cubemap(rc, *cubemap_divisor);
		galaxy_render_from_cubemap(g_galaxy_ogl, rc, 1);
	} else {
		// if (*texture_divisor <= max_texture_divisor || clear) {
			// galaxy_render_to_texture(g_galaxy_tweaks, spiral_trackball.camera, cb, texture_divisor, clear);
			galaxy_render_to_texture(g_galaxy_tweaks, g_galaxy_ogl, bp, spiral_trackball.camera, cb, *texture_divisor, false);
			// texture_divisor = clear ? 1 : *texture_divisor + 1;
			(*texture_divisor)++;
			checkErrors("After draw to texture");
		// }
		// galaxy_render_from_texture(cb, *texture_divisor);
		galaxy_render_from_texture(g_galaxy_ogl, cb, 1);
	}
	clear_active_accumulation_buffer = false;
}

void spiral_scene_render()
{
	galaxy_demo_render(g_galaxy_tweaks, rcube, cbuffer, &cubemap_divisor, &texture_divisor, spiral_trackball.camera);
	if (show_tweaks)
		meter_draw_all(&g_galaxy_meters);
	checkErrors("After meter_draw_all");
	glBindVertexArray(0);
}
