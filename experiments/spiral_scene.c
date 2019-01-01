#include "spiral_scene.h"
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
#include "meter/meter_ogl_renderer.h"
#include "deferred_framebuffer.h"
#include "graphics.h"

#include <glla/glla.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
//#include <SDL2_image/SDL_image.h>

/* Implementing scene "interface" */

SCENE_IMPLEMENT(spiral);

static float screen_width = 640, screen_height = 480;
static int mouse_x = 0, mouse_y = 0;
static struct trackball spiral_trackball;
static struct color_buffer cbuffer;
static struct renderable_cubemap rcube;
static bool clear_accum = false;
static int accum_frames = 0;
static float tweaks[8] = {1, 1, 1, 1};
static float g_absorption[4] = {0.2, 0.1, 0.01, 0.0};
static int max_accum_frames = 180;
static bool accumulate = true;
static bool show_tweaks = true;
static struct {
	float screen;
	float screen_focal;
	float cubemap;
	float cubemap_focal;
} field_of_view;

/* Lua Config */
extern lua_State *L;

/* OpenGL Variables */

static GLint POS_ATTR = 1;
static struct spiral_uniforms {
	GLuint RESOLUTION, MOUSE, TIME, FOCAL, DIR, EYE, BRIGHT, ROTATION, TWEAKS, TWEAKS2, BULGE, ABSORB;
	GLuint DRESOLUTION, DAB, DNFRAMES;
	GLuint DAB_CUBE, DNFRAMES_CUBE;
} UNIF;
static GLuint SHADER, DSHADER, DSHADER_CUBE, VAO, VBO;
static GLfloat vertices[] = {-1, -1, 1, -1, -1, 1, 1, 1};
static float g_brightness;
static float g_rotation;
static float g_bulge_height;
static float g_bulge_width;
static float g_bulge_mask_radius;

static void meter_clear_accum_callback(char *name, enum meter_state state, float value, void *context)
{
	clear_accum = true;
}

int spiral_shader_init(struct spiral_uniforms *unif)
{
	/* Shader initialization */

	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("glsl330", "#version 330");

	char *vsh_key = getglobstr(L, "spiral_vsh_key", "");
	char *fsh_key = getglobstr(L, "spiral_fsh_key", "");

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "versions.glsl330", vsh_key),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "versions.glsl330", "common.noise.GL33", fsh_key),
	}, shader_deferred[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "versions.glsl330", vsh_key),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "versions.glsl330", "spiral.deferred.GL33"),
	}, shader_deferred_cube[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "versions.glsl330", vsh_key),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "versions.glsl330", "spiral.deferred.cubemap.GL33"),
	};
	SHADER = glsw_new_shader_program(shader, LENGTH(shader));
	checkErrors("After creating spiral shader program");
	DSHADER = glsw_new_shader_program(shader_deferred, LENGTH(shader_deferred));
	checkErrors("After creating deferred shader program");
	DSHADER_CUBE = glsw_new_shader_program(shader_deferred_cube, LENGTH(shader_deferred_cube));
	checkErrors("After creating cubemap shader program");

	glswShutdown();
	free(vsh_key);
	free(fsh_key);

	if (!SHADER || !DSHADER) {
		spiral_scene_deinit();
		return -1;
	}

	/* Retrieve uniform variable handles */

	unif->RESOLUTION  = glGetUniformLocation(SHADER, "iResolution");
	unif->MOUSE       = glGetUniformLocation(SHADER, "iMouse");
	unif->TIME        = glGetUniformLocation(SHADER, "iTime");
	unif->FOCAL       = glGetUniformLocation(SHADER, "iFocalLength");
	unif->DIR         = glGetUniformLocation(SHADER, "dir_mat");
	unif->EYE         = glGetUniformLocation(SHADER, "eye_pos");
	unif->BRIGHT      = glGetUniformLocation(SHADER, "brightness");
	unif->ROTATION    = glGetUniformLocation(SHADER, "rotation");
	unif->TWEAKS      = glGetUniformLocation(SHADER, "tweaks");
	unif->TWEAKS2     = glGetUniformLocation(SHADER, "tweaks2");
	unif->BULGE       = glGetUniformLocation(SHADER, "bulge");
	unif->ABSORB      = glGetUniformLocation(SHADER, "absorption");
	checkErrors("After getting uniform handles");

	unif->DRESOLUTION = glGetUniformLocation(DSHADER, "iResolution");
	unif->DAB         = glGetUniformLocation(DSHADER, "accum_buffer");
	unif->DNFRAMES    = glGetUniformLocation(DSHADER, "num_frames_accum");
	checkErrors("After getting deferred uniform handles");

	unif->DAB_CUBE      = glGetUniformLocation(DSHADER_CUBE, "accum_cube");
	unif->DNFRAMES_CUBE = glGetUniformLocation(DSHADER_CUBE, "num_frames_accum");
	checkErrors("After getting cubemap uniform handles");
	glUseProgram(SHADER);

	field_of_view.screen = M_PI/2;
	field_of_view.screen_focal = 1.0/tan(field_of_view.screen/2.0);
	field_of_view.cubemap = M_PI/2; //Cubemap needs to have 90 degree FOV for each face.
	field_of_view.cubemap_focal = 1.0/tan(field_of_view.cubemap/2.0);

	return 0;
}

int spiral_scene_init()
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	if (spiral_shader_init(&UNIF))
		return -1;

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

	/* Set up meter module */
	float y_offset = 25.0;
	meter_init(screen_width, screen_height, 20, meter_ogl_renderer);
	struct widget_meter_style wstyles = {.width = 200, .height = 20, .padding = 2.0};
	struct widget_meter_color tweak_colors = {.fill = {79, 150, 167, 255}, .border = {37, 95, 65, 255}, .font = {255, 255, 255, 255}};
	struct widget_meter_color bulge_colors = {.fill = {179, 95, 107, 255}, .border = {37, 95, 65, 255}, .font = {255, 255, 255, 255}};
	widget_meter widgets[] = {
		{
			.name = "Brightness", .x = 5.0, .y = 0, .min = 0.0, .max = 100.0, .value = getglob(L, "spiral_brightness", 5.0),
			.callback = meter_clear_accum_callback, .target = &g_brightness, 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Rotation", .x = 5.0, .y = 0, .min = 0.0, .max = 1000.0, .value = getglob(L, "spiral_rotation", 400.0),
			.callback = meter_clear_accum_callback, .target = &g_rotation,
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Arm Width", .x = 5.0, .y = 0, .min = 0.0, .max = 32.0, .value = getglob(L, "arm_width", 2.0),
			.callback = meter_clear_accum_callback, .target = &tweaks[5],
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Noise Scale", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0, .value = getglob(L,"noise_scale", 9.0),
			.callback = meter_clear_accum_callback, .target = &tweaks[0],
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Noise Strength", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0, .value = getglob(L,"noise_strength", 9.0),
			.callback = meter_clear_accum_callback, .target = &tweaks[1],
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Diffuse Step Distance", .x = 5.0, .y = 0, .min = -10.0, .max = 10.0, .value = getglob(L,"light_step_distance", 0.2),
			.callback = meter_clear_accum_callback, .target = &tweaks[3],
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Diffuse Intensity", .x = 5.0, .y = 0, .min = 0.000001, .max = 1.0, .value = getglob(L,"diffuse_intensity", 1.0),
			.callback = meter_clear_accum_callback, .target = &tweaks[4],
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Emission Strength", .x = 5.0, .y = 0, .min = 0.000001, .max = 15.0, .value = getglob(L,"emission_strength", 1.0),
			.callback = meter_clear_accum_callback, .target = &tweaks[7],
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Bulge Mask Radius", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0, .value = getglob(L,"bulge_mask_radius", 9.0),
			.callback = meter_clear_accum_callback, .target = &g_bulge_mask_radius,
			.style = wstyles, .color = bulge_colors
		},
		{
			.name = "Bulge Mask Power", .x = 5.0, .y = 0, .min = 0.0, .max = 8.0, .value = getglob(L,"bulge_mask_power", 1.0),
			.callback = meter_clear_accum_callback, .target = &tweaks[6],
			.style = wstyles, .color = bulge_colors
		},
		{
			.name = "Bulge Height", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0, .value = getglob(L,"bulge_height", 20.0),
			.callback = meter_clear_accum_callback, .target = &g_bulge_height,
			.style = wstyles, .color = bulge_colors
		},
		{
			.name = "Bulge Width", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0, .value = getglob(L,"bulge_width", 10.0),
			.callback = meter_clear_accum_callback, .target = &g_bulge_width,
			.style = wstyles, .color = bulge_colors
		},
		{
			.name = "Spiral Density", .x = 5.0, .y = 0, .min = 0.0, .max = 4.0, .value = getglob(L,"spiral_density", 1.0),
			.callback = meter_clear_accum_callback, .target = &tweaks[2],
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Absorption R", .x = 5.0, .y = 0, .min = 0.0, .max = 1.0, .value = getglob(L,"absorption_r", g_absorption[0]),
			.callback = meter_clear_accum_callback, .target = &g_absorption[0],
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Absorption G", .x = 5.0, .y = 0, .min = 0.0, .max = 1.0, .value = getglob(L,"absorption_g", g_absorption[1]),
			.callback = meter_clear_accum_callback, .target = &g_absorption[1],
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Absorption B", .x = 5.0, .y = 0, .min = 0.0, .max = 1.0, .value = getglob(L,"absorption_b", g_absorption[2]),
			.callback = meter_clear_accum_callback, .target = &g_absorption[2],
			.style = wstyles, .color = tweak_colors
		},
	};
	for (int i = 0; i < LENGTH(widgets); i++) {
		widget_meter *w = &widgets[i];
		meter_add(w->name, w->style.width, w->style.height, w->min, w->value, w->max);
		meter_target(w->name, w->target);
		meter_position(w->name, w->x, w->y + y_offset);
		meter_callback(w->name, w->callback, w->callback_context);
		meter_style(w->name, w->color.fill, w->color.border, w->color.font, w->style.padding);
		y_offset += 25;
	}

	accum_frames = 0;
	max_accum_frames = getglob(L, "max_accum_frames", 180);
	accumulate = getglobbool(L, "accumulate", true);

	return 0;
}

void spiral_scene_resize(float width, float height)
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

void spiral_scene_deinit()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(SHADER);
	color_buffer_delete(cbuffer);
	meter_deinit();
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
		button = !meter_mouse(mouse_x, mouse_y, button) && button;
	int scroll_x = input_mouse_wheel_sum.wheel.x, scroll_y = input_mouse_wheel_sum.wheel.y;
	if (trackball_step(&spiral_trackball, mouse_x, mouse_y, button, scroll_x, scroll_y))
		clear_accum = true;

	if (key_pressed(SDL_SCANCODE_TAB))
		show_tweaks = !show_tweaks;
}

void spiral_scene_render()
{
	glBindVertexArray(VAO);
	glUseProgram(SHADER);
	glClearColor(0, 0, 0, 0);

	//Draw in wireframe if 'z' is held down.
	if (key_state[SDL_SCANCODE_Z])
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	bool draw_to_cubemap = getglobbool(L, "cubemap_mode", false) != key_state[SDL_SCANCODE_C];

	if (draw_to_cubemap)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rcube.fbo);
		glUniform1f(UNIF.FOCAL, field_of_view.cubemap_focal);
	} else {
		// Draw to accumulation buffer.
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, cbuffer.fbo);
		glUniform1f(UNIF.FOCAL, field_of_view.screen_focal);
	}

	if (clear_accum || !accumulate) {
		clear_accum = false;
		accum_frames = 0;
		if (draw_to_cubemap) {
			for (int i = 0; i < 6; i++) {
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, rcube.texture, 0);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			}
		} else {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}
	}

	if (accum_frames < max_accum_frames || !accumulate) {
		glUniform1f(UNIF.ROTATION, g_rotation);
		glUniform1f(UNIF.BRIGHT, g_brightness);
		glUniform2f(UNIF.TIME, SDL_GetTicks() / 1000.0, accum_frames);
		glUniform4f(UNIF.MOUSE, mouse_x, mouse_y, 0, 0);
		glUniform4fv(UNIF.TWEAKS, 1, tweaks);
		glUniform4fv(UNIF.TWEAKS2, 1, tweaks + 4);
		glUniform4f(UNIF.BULGE, g_bulge_height, g_bulge_width, g_bulge_mask_radius, g_bulge_width * g_bulge_width);
		glUniform4fv(UNIF.ABSORB, 1, g_absorption);
		glUniform3f(UNIF.EYE, VEC3_COORDS(spiral_trackball.camera.t));
		{
			float dir_mat[9];
			mat3_to_array_cm(spiral_trackball.camera.a, dir_mat);
			glUniformMatrix3fv(UNIF.DIR, 1, GL_FALSE, dir_mat);
			checkErrors("After sending dir_mat");
		}
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		//Each accumulate step, blend additively.
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquation(GL_FUNC_ADD);
		glDepthMask(GL_FALSE);
		if (draw_to_cubemap) {
			glBindFramebuffer(GL_FRAMEBUFFER, rcube.fbo);
			glViewport(0, 0, rcube.width, rcube.width);
			glUniform2f(UNIF.RESOLUTION, rcube.width, rcube.width);

			float s90 = 1.0, c90 = 0.0, s180 = 0.0, c180 = -1.0, s270 = -1.0, c270 = 0.0;
			mat3 *c = &spiral_trackball.camera.a;
			mat3 cubemap_mats[] = {
				mat3_mult(*c, mat3_rotmaty(s90, c90)), // +X
				mat3_mult(*c, mat3_rotmaty(s270, c270)), // -X
				mat3_mult(*c, mat3_rotmatx(s90, c90)), // +Y
				mat3_mult(*c, mat3_rotmatx(s270, c270)), // -Y
				mat3_mult(*c, mat3_rotmaty(s180, c180)), // +Z
				*c, // -Z
			};

			for (int i = 0; i < LENGTH(cubemap_mats); i++) {
				// For each direction, draw to cubemap
				float dir_mat[9];
				mat3_to_array_cm(cubemap_mats[i], dir_mat);
				glUniformMatrix3fv(UNIF.DIR, 1, GL_FALSE, dir_mat);

				//Bind cubemap face texture and draw
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, rcube.texture, 0);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}

			glViewport(0, 0, screen_width, screen_height);
		} else {
			glBindFramebuffer(GL_FRAMEBUFFER, cbuffer.fbo);
			glUniform2f(UNIF.RESOLUTION, screen_width, screen_height);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		accum_frames++;
	}

	// Draw from accumulation buffer to screen.
	if (draw_to_cubemap) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, rcube.fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glDisable(GL_BLEND);
		glUseProgram(DSHADER_CUBE);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, rcube.texture);
		checkErrors("After bind texture");
		glUniform1i(UNIF.DAB_CUBE, 0);
		//The result is divided by the number of accumulated frames, averaging the passes.
		glUniform1i(UNIF.DNFRAMES_CUBE, accum_frames);
		// glUniform2f(UNIF.DRESOLUTION, rcube.width, rcube.width);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	} else {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, cbuffer.fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glDisable(GL_BLEND);
		glUseProgram(DSHADER);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cbuffer.texture);
		glUniform1i(UNIF.DAB, 0);
		//The result is divided by the number of accumulated frames, averaging the passes.
		glUniform1i(UNIF.DNFRAMES, accum_frames);
		glUniform2f(UNIF.DRESOLUTION, screen_width, screen_height);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
	checkErrors("After draw");
	if (show_tweaks)
		meter_draw_all();
	checkErrors("After meter_draw_all");
	glBindVertexArray(0);
}
