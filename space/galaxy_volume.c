#include "graphics.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "galaxy_volume.h"
#include "buffer_group.h"
#include "macros.h"
#include "meter/meter.h"
#include "meter/meter_ogl_renderer.h"
#include "math/utility.h"
#include "experiments/deferred_framebuffer.h"

/* Lua Config */
extern lua_State *L;

GLfloat galcube_positions[] = {
	1.000000, 1.000000, -1.000000,
	-1.000000, 1.000000, -1.000000,
	-1.000000, -1.000000, -1.000000,
	0.999999, -1.000001, 1.000000,
	-1.000000, -1.000000, 1.000000,
	-1.000000, 1.000000, 1.000000,
	1.000000, -1.000000, -1.000000,
	1.000000, 0.999999, 1.000000,
};

GLuint galcube_indices_adjacent[] = {
	0, 5, 1, 5, 2, 6,
	3, 2, 4, 2, 5, 7,
	6, 2, 3, 5, 7, 0,
	2, 5, 4, 5, 3, 6,
	2, 0, 1, 0, 5, 4,
	5, 2, 1, 2, 0, 7,
	6, 7, 0, 1, 2, 3,
	7, 6, 3, 4, 5, 0,
	0, 2, 6, 3, 7, 5,
	6, 0, 2, 4, 3, 7,
	4, 3, 2, 1, 5, 3,
	7, 3, 5, 1, 0, 6,
};

GLuint galcube_indices[] = {
	0, 1, 2,
	3, 4, 5,
	6, 3, 7,
	2, 4, 3,
	2, 1, 5,
	5, 1, 0,
	6, 0, 2,
	7, 3, 5,
	0, 6, 7,
	6, 2, 3,
	4, 2, 5,
	7, 5, 0,
};

struct galaxy_tweaks galaxy_load_tweaks(lua_State *L, const char *tweaks_table_name)
{
	if (lua_getglobal(L, tweaks_table_name) != LUA_TTABLE) {
		printf("Global with name %s is not a table!\n", tweaks_table_name);
		lua_newtable(L);
	}

	#define load_val(value_name, default_value) .value_name = getopttopfield(L, #value_name, default_value)
	struct galaxy_tweaks gt = {
		load_val(brightness, 5.0),
		load_val(rotation, 400.0),
		load_val(arm_width, 2.0),
		load_val(noise_scale, 9.0),
		load_val(noise_strength, 9.0),
		load_val(light_step_distance, 0.2),
		load_val(diffuse_intensity, 1.0),
		load_val(emission_strength, 1.0),
		load_val(bulge_mask_radius, 9.0),
		load_val(bulge_mask_power, 1.0),
		load_val(disk_height, 20.0),
		load_val(bulge_width, 10.0),
		load_val(spiral_density, 1.0),
		load_val(samples, 10.0),
		load_val(freshness, 0.04),
		.light_absorption = {
			getopttopfield(L, "light_absorption_r", 0.2),
			getopttopfield(L, "light_absorption_g", 0.1),
			getopttopfield(L, "light_absorption_b", 0.01),
		}
	};
	#undef load_val

	lua_settop(L, 0);
	return gt;
}

void galaxy_meters_init(lua_State *L, meter_ctx *M, struct galaxy_tweaks *gt, float screen_width, float screen_height, meter_callback_fn clear_callback)
{
	/* Set up meter module */
	float y_offset = 25.0;
	meter_init(M, screen_width, screen_height, 20, meter_ogl_renderer);
	struct widget_meter_style wstyles = {.width = 200, .height = 20, .padding = 2.0};
	struct widget_meter_color tweak_colors = {.fill = {79, 150, 167, 255}, .border = {37, 95, 65, 255}, .font = {255, 255, 255, 255}};
	struct widget_meter_color bulge_colors = {.fill = {179, 95, 107, 255}, .border = {37, 95, 65, 255}, .font = {255, 255, 255, 255}};
	widget_meter widgets[] = {
		{
			.name = "Brightness", .x = 5.0, .y = 0, .min = 0.0, .max = 100.0,
			.callback = clear_callback, .target = &gt->brightness, 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Rotation", .x = 5.0, .y = 0, .min = 0.0, .max = 1000.0,
			.callback = clear_callback, .target = &gt->rotation,
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Arm Width", .x = 5.0, .y = 0, .min = 0.0, .max = 32.0,
			.callback = clear_callback, .target = &gt->arm_width,
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Noise Scale", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0,
			.callback = clear_callback, .target = &gt->noise_scale,
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Noise Strength", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0,
			.callback = clear_callback, .target = &gt->noise_strength,
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Diffuse Step Distance", .x = 5.0, .y = 0, .min = -10.0, .max = 10.0,
			.callback = clear_callback, .target = &gt->light_step_distance,
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Diffuse Intensity", .x = 5.0, .y = 0, .min = 0.000001, .max = 1.0,
			.callback = clear_callback, .target = &gt->diffuse_intensity,
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Emission Strength", .x = 5.0, .y = 0, .min = 0.000001, .max = 15.0,
			.callback = clear_callback, .target = &gt->emission_strength,
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Bulge Mask Radius", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0,
			.callback = clear_callback, .target = &gt->bulge_mask_radius,
			.style = wstyles, .color = bulge_colors
		},
		{
			.name = "Bulge Mask Power", .x = 5.0, .y = 0, .min = 0.0, .max = 8.0,
			.callback = clear_callback, .target = &gt->bulge_mask_power,
			.style = wstyles, .color = bulge_colors
		},
		{
			.name = "Disk Height", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0,
			.callback = clear_callback, .target = &gt->disk_height,
			.style = wstyles, .color = bulge_colors
		},
		{
			.name = "Bulge Width", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0,
			.callback = clear_callback, .target = &gt->bulge_width,
			.style = wstyles, .color = bulge_colors
		},
		{
			.name = "Spiral Density", .x = 5.0, .y = 0, .min = 0.0, .max = 4.0,
			.callback = clear_callback, .target = &gt->spiral_density,
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Absorption R", .x = 5.0, .y = 0, .min = 0.0, .max = 1.0,
			.callback = clear_callback, .target = &gt->light_absorption[0],
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Absorption G", .x = 5.0, .y = 0, .min = 0.0, .max = 1.0,
			.callback = clear_callback, .target = &gt->light_absorption[1],
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Absorption B", .x = 5.0, .y = 0, .min = 0.0, .max = 1.0,
			.callback = clear_callback, .target = &gt->light_absorption[2],
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Freshness", .x = 5.0, .y = 0, .min = 0.0, .max = 1.0,
			.target = &gt->freshness,
			.style = wstyles, .color = tweak_colors
		},
		{
			.name = "Samples", .x = 5.0, .y = 0, .min = 0.0, .max = 100,
			.target = &gt->samples,
			.style = wstyles, .color = tweak_colors
		}
	};
	for (int i = 0; i < LENGTH(widgets); i++) {
		widget_meter *w = &widgets[i];
		meter_add(M, w->name, w->style.width, w->style.height, w->min, *w->target, w->max);
		meter_target(M, w->name, w->target);
		meter_position(M, w->name, w->x, w->y + y_offset);
		meter_callback(M, w->name, w->callback, w->callback_context);
		meter_style(M, w->name, w->color.fill, w->color.border, w->color.font, w->style.padding);
		y_offset += 25;
	}
}

int galaxy_shader_init(struct galaxy_ogl *gal)
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
	gal->shader = glsw_new_shader_program(shader, LENGTH(shader));
	checkErrors("After creating spiral shader program");
	gal->dshader = glsw_new_shader_program(shader_deferred, LENGTH(shader_deferred));
	checkErrors("After creating deferred shader program");
	gal->dshader_cube = glsw_new_shader_program(shader_deferred_cube, LENGTH(shader_deferred_cube));
	checkErrors("After creating cubemap shader program");

	glswShutdown();
	free(vsh_key);
	free(fsh_key);

	if (!gal->shader || !gal->dshader) {
		glDeleteProgram(gal->shader);
		glDeleteProgram(gal->dshader);
		return -1;
	}

	/* Retrieve uniform variable handles */

	gal->unif.resolution  = glGetUniformLocation(gal->shader, "iResolution");
	gal->unif.mouse       = glGetUniformLocation(gal->shader, "iMouse");
	gal->unif.time        = glGetUniformLocation(gal->shader, "iTime");
	gal->unif.focal       = glGetUniformLocation(gal->shader, "iFocalLength");
	gal->unif.dir         = glGetUniformLocation(gal->shader, "dir_mat");
	gal->unif.eye         = glGetUniformLocation(gal->shader, "eye_pos");
	gal->unif.bright      = glGetUniformLocation(gal->shader, "brightness");
	gal->unif.rotation    = glGetUniformLocation(gal->shader, "rotation");
	gal->unif.tweaks      = glGetUniformLocation(gal->shader, "tweaks1");
	gal->unif.tweaks2     = glGetUniformLocation(gal->shader, "tweaks2");
	gal->unif.bulge       = glGetUniformLocation(gal->shader, "bulge");
	gal->unif.absorb      = glGetUniformLocation(gal->shader, "absorption");
	gal->unif.fresh       = glGetUniformLocation(gal->shader, "freshness");
	gal->unif.samples     = glGetUniformLocation(gal->shader, "samples");
	checkErrors("After getting uniform handles");

	gal->unif.dresolution = glGetUniformLocation(gal->dshader, "iResolution");
	gal->unif.dab         = glGetUniformLocation(gal->dshader, "accum_buffer");
	gal->unif.dnframes    = glGetUniformLocation(gal->dshader, "num_frames_accum");
	checkErrors("After getting deferred uniform handles");

	gal->unif.dab_cube      = glGetUniformLocation(gal->dshader_cube, "accum_cube");
	gal->unif.dnframes_cube = glGetUniformLocation(gal->dshader_cube, "num_frames_accum");
	checkErrors("After getting cubemap uniform handles");
	glUseProgram(gal->shader);

	return 0;
}

int buffer_galaxy_cube(struct buffer_group bg)
{
	glBindBuffer(GL_ARRAY_BUFFER, bg.vbo);
	checkErrors("After bind vbo");
	glBufferData(GL_ARRAY_BUFFER, sizeof(galcube_positions), galcube_positions, GL_STATIC_DRAW);
	checkErrors("After buffer positions");
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.aibo);
	checkErrors("After bind aibo");
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(galcube_indices_adjacent), galcube_indices_adjacent, GL_STATIC_DRAW);
	checkErrors("After buffer galcube_indices_adjacent");
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);
	checkErrors("After bind ibo");
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(galcube_indices), galcube_indices, GL_STATIC_DRAW);
	checkErrors("After buffer galcube_indices");
	// printf("galcube_positions size: %lu, galcube_indices size: %lu, galcube_indices_adjacent size: %lu\n", sizeof(galcube_positions), sizeof(galcube_indices), sizeof(galcube_indices_adjacent));
	// puts("galcube_positions:");
	// for (int i = 0; i < LENGTH(galcube_positions); i++)
	// 	printf("%f, ", galcube_positions[i]);
	// puts("galcube_indices:");
	// for (int i = 0; i < LENGTH(galcube_indices); i++)
	// 	printf("%u, ", galcube_indices[i]);
	// puts("\n");
	return LENGTH(galcube_indices);
}