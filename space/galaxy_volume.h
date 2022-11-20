#ifndef GALAXY_VOLUME_H
#define GALAXY_VOLUME_H

#include "glla.h"
#include "meter/meter.h"
#include "buffer_group.h"
#include "experiments/deferred_framebuffer.h"
#include "luaengine/lua_configuration.h"

struct blend_params {
	GLenum blendfn, srcfact, dstfact;
	float blendcol[4];
};

struct galaxy_tweaks {
	float light_absorption[4];
	float field_of_view;
	float focal_length;
	float brightness;
	float rotation;
	float diameter;

	union {
		struct {
			float arm_width;
			float noise_scale;
			float noise_strength;
			float spiral_density;
		};
		float tweaks1[4];
	};
	union {		
		struct {
			float diffuse_intensity;
			float light_step_distance;
			float emission_strength;
			float disk_height;
		};
		float tweaks2[4];
	};
	union {
		struct {
			float bulge_width;
			float bulge_mask_radius;
			float bulge_mask_power;
			float bulge_width_squared;
		};
		float bulge[4];
	};
	float freshness;
	float samples;
	float render_dist;
};

struct galaxy_ogl {
	struct {
		GLint pos; //location = 1
	} attr;
	struct {
		GLuint resolution, mouse, time, focal, dir, eye, bright, rotation, diameter, tweaks, tweaks2, bulge, absorb, fresh, samples, render_dist;
		GLuint dresolution, dab, dnframes;
		GLuint dab_cube, dnframes_cube;
	} unif;
	GLuint shader, dshader, dshader_cube, vao, vbo;
};

int buffer_galaxy_cube(struct buffer_group bg);

struct galaxy_tweaks galaxy_load_tweaks(lua_State *L, const char *tweaks_table_name);
int galaxy_shader_init(struct galaxy_ogl *gal);
void galaxy_meters_init(lua_State *L, meter_ctx *M, struct galaxy_tweaks *gt, float screen_width, float screen_height, meter_callback_fn clear_callback);
void galaxy_bind_cubemap();
int galaxy_no_worries_just_render_cubemap(bool clear);
void galaxy_render_to_cubemap(struct galaxy_tweaks gt, struct galaxy_ogl gal, struct blend_params bp, amat4 camera, struct renderable_cubemap rc, int framecount, bool clear_first);
void galaxy_render_from_cubemap(struct galaxy_ogl gal, struct renderable_cubemap rc, float divisor);
void galaxy_render_to_texture(struct galaxy_tweaks gt, struct galaxy_ogl gal, struct blend_params bp, amat4 camera, struct color_buffer cb, int framecount, bool clear_first);
void galaxy_render_from_texture(struct galaxy_ogl gal, struct color_buffer cb, float divisor);
void galaxy_demo_render(struct galaxy_tweaks gt, struct renderable_cubemap rc, struct color_buffer cb, int *cubemap_divisor, int *texture_divisor, amat4 camera);

#endif