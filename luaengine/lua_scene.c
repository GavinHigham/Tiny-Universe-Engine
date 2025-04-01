#define SCENE_HAS_FILEDROP
#include "scene.h"
#include "lua_configuration.h"
#include "drawf.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "graphics.h"
#include "input_event.h"
#include "macros.h"
#include "math/utility.h"
#include "lua_scene.h"
#include "shader_utils.h"
#include "trackball/trackball.h"
#include "space/triangular_terrain_tile.h"
#include "meter/meter.h"
#include "meter/meter_ogl_renderer.h"
#include <assert.h>
#include <glla.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/*
This will be the scene used to create scenes entirely from Lua - it will hand control off to a Lua script.

Initially, the Lua script will have to implement the scene interface (init, deinit, resize, update, render).
Later, Lua wrappers for OpenGL handles will ensure they are generated and deleted as necessary.
*/

#define USING_ROWS

/* Implementing scene "interface" */

SCENE_IMPLEMENT(lua);

static float screen_width = 640, screen_height = 480;
static int mouse_x = 0, mouse_y = 0;

// Temporary for an experiment, should go away when I refactor the meter logic a bit
static float dither_transparency = 1.0;

static float proj_mat_near = 0.1;
static float proj_mat_far = 10000000;
static float proj_mat[16];

/* Lua Config */
extern lua_State *L;
int luaopen_l_opengl(lua_State *L);
int luaopen_l_glla(lua_State *L);
int luaopen_l_sdl_input(lua_State *L);
int luaopen_l_image(lua_State *L);
void l_mat4_push(lua_State *L, float a[16]);

/* Atmosphere stuff Frankenstein'd in */
#include "experiments/atmosphere/atmosphere_scene.h"

//Just reuse the ones from atmosphere_scene for now
//The plan is for this stuff to go away as I expose/move more functionality to Lua
struct atmosphere_tweaks g_luascene_tweaks;
meter_ctx g_luascene_meters;

extern struct atmosphere_tweaks atmosphere_load_tweaks(lua_State *L, const char *tweaks_table_name);

void lua_scene_meter_callback(char *name, enum meter_state state, float value, void *context)
{
	lua_State *L = context;
	int top = lua_gettop(L);
	//TODO: Find a better way to pass these variables back and forth.
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene_meters");
	lua_pushstring(L, name);
	lua_pushnumber(L, value);
	lua_settable(L, -3);
	lua_settop(L, top);
}

//Just copy and rename this function because it refers to the static proj_mat
void lua_scene_fov_updated(char *name, enum meter_state state, float value, void *context)
{
	struct atmosphere_tweaks *at = context;
	lua_State *L = at->extra_context;
	at->focal_length = fov_to_focal(value);
	make_projection_matrix(g_luascene_tweaks.fov, screen_width/screen_height, proj_mat_near, proj_mat_far, proj_mat);
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene");
	l_mat4_push(L, proj_mat);
	lua_setfield(L, -2, "proj_mat");
	lua_pop(L, 1);
	lua_scene_meter_callback(name, state, value, L);
}

void lua_scene_filedrop(const char *file) {
	int top = lua_gettop(L);
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene");
	lua_getfield(L, -1, "onfiledrop");
	lua_pushstring(L, file);
	if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
		printf("%s\n", lua_tostring(L, -1));
	}
	lua_settop(L, top);
}

//TODO: First wire this all up to set Lua globals, and set the uniforms from Lua.
//Then, make it possible to create tweaks from Lua so I can remove the initialization here entirely.
void lua_scene_meters_init(lua_State *L, meter_ctx *M, struct atmosphere_tweaks *at, float screen_width, float screen_height)
{
	/* Set up meter module */
	float y_offset = 25.0;
	meter_init(M, screen_width, screen_height, 20, meter_ogl_renderer);
	struct widget_meter_style wstyles = {.width = 600, .height = 20, .padding = 2.0};
	widget_meter widgets[] = {
		{
			.name = "Planet Radius", .x = 5.0, .y = 0, .min = 0.1, .max = 5.0,
			.target = &at->planet_radius,
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Atmosphere Height", .x = 5.0, .y = 0, .min = 0.0001, .max = 2.0,
			.target = &at->atmosphere_height,
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Field of View", .x = 5.0, .y = 0, .min = 0.01, .max = 2.0*M_PI,
			.target = &at->fov,
			.callback = lua_scene_fov_updated, .callback_context = at,
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Samples along AB", .x = 5.0, .y = 0, .min = 0.0, .max = 30,
			.target = &at->samples_ab, .always_snap = true,
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Samples along CP", .x = 5.0, .y = 0, .min = 0.0, .max = 30,
			.target = &at->samples_cp, .always_snap = true,
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Poke and Prod Variable", .x = 5.0, .y = 0, .min = -1.0, .max = 1.0,
			.target = &at->p_and_p,
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Air Particle Interaction Transmittance R", .x = 5.0, .y = 0, .min = 0.0, .max = 0.00003,
			.target = &at->air_b[0],
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Air Particle Interaction Transmittance G", .x = 5.0, .y = 0, .min = 0.0, .max = 0.00003,
			.target = &at->air_b[1],
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Air Particle Interaction Transmittance B", .x = 5.0, .y = 0, .min = 0.0, .max = 0.00003,
			.target = &at->air_b[2],
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Scale Height", .x = 5.0, .y = 0, .min = 0.0, .max = 100000,
			.target = &at->scale_height,
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
		{
			.name = "Planet Scale", .x = 5.0, .y = 0, .min = 0.0, .max = 10000000,
			.target = &at->planet_scale,
			.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		},
	};
	for (int i = 0; i < LENGTH(widgets); i++) {
		widget_meter *w = &widgets[i];
		//TODO: Make it possible to define these without having a target (we NULL-deref in that case currently)
		meter_add(M, w->name, w->style.width, w->style.height, w->min, *w->target, w->max);
		meter_target(M, w->name, w->target);
		meter_position(M, w->name, w->x, w->y + y_offset);
		if (w->callback)
			meter_callback(M, w->name, w->callback, w->callback_context);
		else
			meter_callback(M, w->name, lua_scene_meter_callback, L);
		meter_style(M, w->name, w->color.fill, w->color.border, w->color.font, w->style.padding, 0);
		meter_always_snap(M, w->name, w->always_snap);
		y_offset += 25;
	}
}

/* End Atmosphere stuff */

int lua_scene_init(bool reload)
{
	luaconf_register_builtin_lib(L, luaopen_l_opengl, "OpenGL");
	luaconf_register_builtin_lib(L, luaopen_l_glla, "glla");
	luaconf_register_builtin_lib(L, luaopen_l_sdl_input, "input");
	luaconf_register_builtin_lib(L, luaopen_l_image, "image");

	/* Retrieve scene table and save it to Lua registry */
	int top = lua_gettop(L);
	lua_getglobal(L, "require");
	lua_getglobal(L, "lua_scene");
	if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
		printf("%s\n", lua_tostring(L, -1));
		return -1;
	}

	// if (luaL_dostring(L, "return require(lua_scene)")) {
	// 	printf("Could not load the lua_scene\n");
	// 	return -1;
	// }

	lua_setfield(L, LUA_REGISTRYINDEX, "tu_lua_scene");
	/* Call the init function */
	int result = lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene") != LUA_TTABLE;
	if (result) {
		printf("tu_lua_scene is not a table\n");
		goto unsave_package;
	}

	//Get the meters table set up before, in case I use it in init.
	lua_newtable(L);
	lua_pushvalue(L, -1);
	lua_setfield(L, -3, "meters");
	lua_setfield(L, LUA_REGISTRYINDEX, "tu_lua_scene_meters");

	result = lua_getfield(L, -1, "init") != LUA_TFUNCTION;
	if (result) {
		printf("No \"init\" function found in Lua Scene table.\n");
		goto unsave_package;
	}
	lua_pushboolean(L, reload);

	//If there's no error return value, a nil is pushed
	result = lua_pcall(L, 1, 1, 0) != LUA_OK;
	if (result) {
		//Handle Lua errors
		if (lua_isstring(L, -1))
			printf("%s\n", lua_tostring(L, -1));
		goto unsave_package;
	}

	//Check for errors indicated by return value
	//TODO: Allow string errors as well
	result = lua_tointeger(L, -1);

	unsave_package:
	if (result) {
		//Unsave the package so the user has a chance to fix and reload it
		luaL_dostring(L, "package.loaded[lua_scene] = nil");
		return result;
	}

	lua_settop(L, top);

	g_luascene_tweaks = atmosphere_load_tweaks(L, "atmosphere_defaults");
	g_luascene_tweaks.extra_context = L;
	g_luascene_tweaks.show_tweaks = false;
	lua_scene_meters_init(L, &g_luascene_meters, &g_luascene_tweaks, screen_width, screen_height);

	return 0;
}

void lua_scene_resize(float width, float height)
{
	/* Call Lua script resize function */
	int top = lua_gettop(L);
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene");
	lua_getfield(L, -1, "resize");
	lua_pushnumber(L, width);
	lua_pushnumber(L, height);
	lua_pcall(L, 2, 0, 0);
	lua_settop(L, top);

	printf("Resizing! width: %f, height: %f\n", width, height);
	glViewport(0, 0, width, height);
	screen_width = width;
	screen_height = height;
	meter_resize_screen(&g_luascene_meters, screen_width, screen_height);
	make_projection_matrix(g_luascene_tweaks.fov, screen_width/screen_height, proj_mat_near, proj_mat_far, proj_mat);
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene");
	l_mat4_push(L, proj_mat);
	lua_setfield(L, -2, "proj_mat");
	lua_pushnumber(L, 2.0/log2(proj_mat_far + 1.0));
	lua_setfield(L, -2, "log_depth_intermediate_factor");
	lua_pop(L, 1);
}

void lua_scene_deinit()
{
	/* Call Lua script deinit function */
	int top = lua_gettop(L);
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene");
	lua_getfield(L, -1, "deinit");
	lua_pcall(L, 0, 0, 0);

	//package.loaded[lua_scene] = nil
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "loaded");
	lua_getglobal(L, "lua_scene");
	lua_pushnil(L);
	lua_settable(L, -3);
	lua_settop(L, top);

	luaconf_unregister_builtin_lib(L, "OpenGL");
	luaconf_unregister_builtin_lib(L, "glla");
	luaconf_unregister_builtin_lib(L, "input");
	luaconf_unregister_builtin_lib(L, "image");
}

void lua_scene_update(float dt)
{
	/* Call Lua script update function */
	int top = lua_gettop(L);
	if (lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene") == LUA_TTABLE) {
		lua_getfield(L, -1, "update");
		lua_pushnumber(L, dt);
		if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
			if (lua_isstring(L, -1))
				printf("%s\n", lua_tostring(L, -1));
			return;
		} else {
			int result = lua_tointeger(L, -1);
			if (result)
				return;
		}
	} else {
		printf("tu_lua_scene is not a table\n");
	}
	lua_settop(L, top);

	Uint32 buttons_held = SDL_GetMouseState(&mouse_x, &mouse_y);
	bool button = buttons_held & SDL_BUTTON(SDL_BUTTON_LEFT);

	if (g_luascene_tweaks.show_tweaks)
		button = !meter_mouse_relative(&g_luascene_meters, mouse_x, mouse_y, button,
		key_state[SDL_SCANCODE_LSHIFT] || key_state[SDL_SCANCODE_RSHIFT],
		key_state[SDL_SCANCODE_LCTRL]  || key_state[SDL_SCANCODE_RCTRL]) && button;

	if (key_pressed(SDL_SCANCODE_TAB))
		g_luascene_tweaks.show_tweaks = !g_luascene_tweaks.show_tweaks;
}

void lua_scene_render()
{
	/* Call Lua script render function */
	int top = lua_gettop(L);
	if (lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene") == LUA_TTABLE) {
		lua_getfield(L, -1, "render");
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			if (lua_isstring(L, -1))
				printf("%s\n", lua_tostring(L, -1));
			return;
		} else {
			int result = lua_tointeger(L, -1);
			if (result)
				return;
		}
	} else {
		printf("tu_lua_scene is not a table\n");
	}
	lua_settop(L, top);

	if (g_luascene_tweaks.show_tweaks)
		meter_draw_all(&g_luascene_meters);
}
