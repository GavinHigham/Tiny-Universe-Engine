#include "lua_configuration.h"
#include "drawf.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "graphics.h"
#include "input_event.h"
#include "macros.h"
#include "math/utility.h"
#include "lua_scene.h"
#include "scene.h"
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

static amat4 eye_frame = {.a = MAT3_IDENT, .t = {0, 0, 5}}; 
static amat4 tri_frame = {.a = MAT3_IDENT, .t = {0, 0, 0}};
static amat4 ico_frame = {.a = MAT3_IDENT, .t = {0, 0, 0}};

static float proj_mat[16];
static struct trackball lua_trackball;

#define NUM_ROWS 16
#define VERTS_PER_ROW(rows) ((rows+2)*(rows+1)/2)
static const int rows = NUM_ROWS;
extern int PRIMITIVE_RESTART_INDEX;

/* Lua Config */
extern lua_State *L;
int luaopen_lua_opengl(lua_State *L);

/* OpenGL Variables */

static GLuint ROWS;
static GLuint SHADER, MM, MVPM, TEXSCALE, VBO, INBO;
static GLint SAMPLER0, VLERPS_ATTR;

static GLuint DM, EP, LP, PL, FL, IS, TI, AH, AB, PP, CP, RES, AIRB, SH, PS;
static GLint POSITION_ATTR, TX_ATTR;

// static GLint POS_ATTR[3] = {1,2,3}, TX_ATTR[3] = {4,5,6};
static GLuint test_gl_tx = 0;

//Adapted from http://www.glprogramming.com/red/chapter02.html
static const float x = 0.525731112119133606;
static const float z = 0.850650808352039932;
// static const float ico_v[] = {    
// 	-x,  0, z,  x,  0,  z, -x, 0, -z,  x, 0, -z, 0,  z, x,  0,  z, -x,
// 	 0, -z, x,  0, -z, -x,  z, x,  0, -z, x,  0, z, -x, 0, -z, -x,  0
// };
static const vec3 ico_v[] = {    
	{-x,  0, z}, { x,  0,  z}, {-x, 0, -z}, { x, 0, -z}, {0,  z, x}, { 0,  z, -x},
	{ 0, -z, x}, { 0, -z, -x}, { z, x,  0}, {-z, x,  0}, {z, -x, 0}, {-z, -x,  0}
};

static const unsigned int ico_i[] = {
	1,0,4,   9,4,0,   9,5,4,   8,4,5,   4,8,1,  10,1,8,  10,8,3,  5,3,8,   3,5,2,  9,2,5,  
	3,7,10,  6,10,7,  7,11,6,  0,6,11,  0,1,6,  10,6,1,  0,11,9,  2,9,11,  3,2,7,  11,7,2,
};

//Texture coordinates for each vertex of a face. Pairs of faces share a texture.
static const float ico_tx[] = {
	0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0, 
	0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0, 
	0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0, 
	0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0, 
	0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,  0.0,0.0, 1.0,0.0, 0.0,1.0,  1.0,1.0, 0.0,1.0, 1.0,0.0,
};

typedef struct {
	float x, y, z, u, v;
} vertex;

//times three vertices, times 20 tiles.
static struct instance_attributes {
	float pos[9];
	float tx[6];
} instance_data[20];
//float uniform_data[tu_len * 3 * 20];

/* Atmosphere stuff Frankenstein'd in */
#include "experiments/atmosphere/atmosphere_scene.h"

//Just reuse the ones from atmosphere_scene for now
//The plan is for this stuff to go away as I expose/move more functionality to Lua
extern struct atmosphere_tweaks g_atmosphere_tweaks;
extern meter_ctx g_atmosphere_meters;

//Just copy and rename this function because it refers to the static proj_mat
void lua_scene_fov_updated(char *name, enum meter_state state, float value, void *context)
{
	struct atmosphere_tweaks *at = context;
	at->focal_length = fov_to_focal(value);
	make_projection_matrix(g_atmosphere_tweaks.fov, screen_width/screen_height, 0.1, 200, proj_mat);
}

extern struct atmosphere_tweaks atmosphere_load_tweaks(lua_State *L, const char *tweaks_table_name);
extern void atmosphere_meters_init(lua_State *L, meter_ctx *M, struct atmosphere_tweaks *at, float screen_width, float screen_height);

/* End Atmosphere stuff */

int lua_scene_init()
{
	luaconf_register_builtin_lib(L, luaopen_lua_opengl, "OpenGL");

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
	if (lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene") == LUA_TTABLE) {
		lua_getfield(L, -1, "init");
		if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
			printf("%s\n", lua_tostring(L, -1));
			return -1;
		} else {
			int result = lua_tointeger(L, -1);
			if (result)
				return result;
		}
	} else {
		printf("tu_lua_scene is not a table\n");
	}

	lua_settop(L, top);
	checkErrors("After Lua scene init");

	/* Shader initialization */
	// glswInit();
	// glswSetPath("shaders/glsw/", ".glsl");
	// glswAddDirectiveToken("glsl330", "#version 330");

	// // GLuint shader[] = {
	// // 	glsw_shader_from_keys(GL_VERTEX_SHADER, "versions.glsl330", "common.noise.GL33", "proctri.vertex.GL33"),
	// // 	glsw_shader_from_keys(GL_FRAGMENT_SHADER, "versions.glsl330", "common.noise.GL33", "proctri.fragment.GL33"),
	// // };
	// GLuint shader[] = {
	// 	glsw_shader_from_keys(GL_VERTEX_SHADER, "versions.glsl330", "atmosphere.vertex.GL33"),
	// 	glsw_shader_from_keys(GL_FRAGMENT_SHADER, "versions.glsl330", "atmosphere.fragment.GL33", "common.utility.GL33", "common.noise.GL33", "common.lighting.GL33", "common.debugging.GL33"),
	// };
	// SHADER = glsw_new_shader_program(shader, LENGTH(shader));
	// glswShutdown();
	lua_getglobal(L, "shaderProgram");
	GLuint *program = luaL_testudata(L, -1, "tu.ShaderProgram");
	checkErrors("After getting ShaderProgram");

	if (!program || !*program) {
		lua_scene_deinit();
		return -1;
	}
	SHADER = *program;

	/* Retrieve uniform variable handles */

	MM       = glGetUniformLocation(SHADER, "model_matrix");
	MVPM     = glGetUniformLocation(SHADER, "model_view_projection_matrix");
	// SAMPLER0 = glGetUniformLocation(SHADER, "diffuse_tx");
	ROWS     = glGetUniformLocation(SHADER, "rows");
	// TEXSCALE = glGetUniformLocation(SHADER, "tex_scale");
	checkErrors("After getting uniform handles");

	DM   = glGetUniformLocation(SHADER, "dir_mat");
	EP   = glGetUniformLocation(SHADER, "eye_pos");
	LP   = glGetUniformLocation(SHADER, "light_pos");
	PL   = glGetUniformLocation(SHADER, "planet");
	FL   = glGetUniformLocation(SHADER, "focal_length");
	IS   = glGetUniformLocation(SHADER, "ico_scale");
	AB   = glGetUniformLocation(SHADER, "samples_ab");
	CP   = glGetUniformLocation(SHADER, "samples_cp");
	PP   = glGetUniformLocation(SHADER, "p_and_p");
	TI   = glGetUniformLocation(SHADER, "time");
	AH   = glGetUniformLocation(SHADER, "atmosphere_height");
	RES  = glGetUniformLocation(SHADER, "resolution");
	AIRB = glGetUniformLocation(SHADER, "air_b");
	SH   = glGetUniformLocation(SHADER, "scale_height");
	PS   = glGetUniformLocation(SHADER, "planet_scale");


	/* Vertex data */

	vertex vertices[LENGTH(ico_i)];
	for (int i = 0; i < LENGTH(ico_i); i++) {
		int idx = ico_i[i];
		vertices[i] = (vertex){ico_v[idx].x, ico_v[idx].y, ico_v[idx].z, ico_tx[i*2], ico_tx[i*2+1]};
	}

	// float tri_lerps[2 * VERTS_PER_ROW(NUM_ROWS)];
	// get_tri_lerp_vals(tri_lerps, rows);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// glBufferData(GL_ARRAY_BUFFER, sizeof(tri_lerps), tri_lerps, GL_STATIC_DRAW);

	glGenBuffers(1, &INBO);
	checkErrors("After gen indexed array buffer");

	/* Vertex attributes */

	// int attr_div = 1;
	// for (int i = 0; i < 3; i++)
	// {
	// 	glEnableVertexAttribArray(POS_ATTR[i]);
	// 	glVertexAttribPointer(POS_ATTR[i], 3, GL_FLOAT, GL_FALSE,
	// 		sizeof(struct instance_attributes), (void *)(offsetof(struct instance_attributes, pos) + i*3*sizeof(float)));
	// 	glVertexAttribDivisor(POS_ATTR[i], attr_div);
	// 	checkErrors("After attr divisor for pos");
	// }

	// for (int i = 0; i < 3; i++)
	// {
	// 	glEnableVertexAttribArray(TX_ATTR[i]);
	// 	glVertexAttribPointer(TX_ATTR[i], 2, GL_FLOAT, GL_FALSE,
	// 		sizeof(struct instance_attributes), (void *)(offsetof(struct instance_attributes, tx) + i*2*sizeof(float)));
	// 	glVertexAttribDivisor(TX_ATTR[i], attr_div);
	// 	checkErrors("After attr divisor for attr");
	// }

	// VLERPS_ATTR = 7; //glGetAttribLocation(SHADER, "vlerp"); 
	// glEnableVertexAttribArray(VLERPS_ATTR);

	// checkErrors("After setting attrib divisors");
	// glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// checkErrors("After binding VBO");
	// glVertexAttribPointer(VLERPS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, 0);
	// checkErrors("After setting VBO attrib pointer");

	POSITION_ATTR = glGetAttribLocation(SHADER, "position"); 
	TX_ATTR       = glGetAttribLocation(SHADER, "tx");
	if (POSITION_ATTR == -1 || TX_ATTR == -1) {
		printf("Vertex attributes were not retrieved successfully.\n");
		lua_scene_deinit();
		return -1;
	}

	glEnableVertexAttribArray(POSITION_ATTR);
	glEnableVertexAttribArray(TX_ATTR);
	glVertexAttribPointer(POSITION_ATTR, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, x));
	glVertexAttribPointer(TX_ATTR,       2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, u));

	/* Uniform Buffer */

	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 3; j++) {
			memcpy(&instance_data[i].pos[3*j], &ico_v[ico_i[i*3 + j]*3], 3*sizeof(float));
			memcpy(&instance_data[i].tx[2*j],  &ico_tx[i*6 + j*2],       2*sizeof(float));
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, INBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(instance_data), instance_data, GL_DYNAMIC_DRAW);
	checkErrors("After upload indexed array data");

	/* Misc. OpenGL bits */

	glClearDepth(1);
	glDepthFunc(GL_LESS);
	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);
	glBindVertexArray(0);

	/* For rotating the icosahedron */
	lua_trackball = trackball_new(tri_frame.t, 5.0);
	trackball_set_speed(&lua_trackball, 1.0/200.0, 1.0/200.0, 1/10.0);
	trackball_set_bounds(&lua_trackball, INFINITY, INFINITY, INFINITY, INFINITY);

	// char texture_path[gettmpglobstr(L, "proctri_tex", "grass.png", NULL)];
	//                   gettmpglobstr(L, "proctri_tex", "grass.png", texture_path);
	// test_gl_tx = load_gl_texture(texture_path);
	// glUseProgram(SHADER);
	// glUniform1f(TEXSCALE, getglob(L, "tex_scale", 1.0));
	// glUseProgram(0);

	g_atmosphere_tweaks = atmosphere_load_tweaks(L, "atmosphere_defaults");
	atmosphere_meters_init(L, &g_atmosphere_meters, &g_atmosphere_tweaks, screen_width, screen_height);

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

	glViewport(0, 0, width, height);
	screen_width = width;
	screen_height = height;
	meter_resize_screen(&g_atmosphere_meters, screen_width, screen_height);
	make_projection_matrix(g_atmosphere_tweaks.fov, screen_width/screen_height, 0.1, 200, proj_mat);
}

void lua_scene_deinit()
{
	glDeleteBuffers(1, &VBO);

	/* Call Lua script deinit function */
	int top = lua_gettop(L);
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene");
	lua_getfield(L, -1, "deinit");
	lua_pcall(L, 0, 0, 0);
	lua_settop(L, top);

	luaconf_unregister_builtin_lib(L, "OpenGL");
}

void lua_scene_update(float dt)
{
	/* Call Lua script update function */
	int top = lua_gettop(L);
	lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene");
	lua_getfield(L, -1, "update");
	lua_pushnumber(L, dt);
	lua_pcall(L, 1, 0, 0);
	lua_settop(L, top);

	static bool relative_mouse = false;

	eye_frame.t += mat3_multvec(lua_trackball.camera.a, (vec3){
		key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A],
		key_state[SDL_SCANCODE_E] - key_state[SDL_SCANCODE_Q],
		key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W],
	} * 0.15);

	g_atmosphere_tweaks.scene_time += dt;

	Uint32 buttons = mouse_button_pressed(&mouse_x, &mouse_y);
	if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
		relative_mouse = !relative_mouse;

	Uint32 buttons_held = SDL_GetMouseState(&mouse_x, &mouse_y);
	bool button = buttons_held & SDL_BUTTON(SDL_BUTTON_LEFT);

	if (g_atmosphere_tweaks.show_tweaks)
		button = !meter_mouse_relative(&g_atmosphere_meters, mouse_x, mouse_y, button,
		key_state[SDL_SCANCODE_LSHIFT] || key_state[SDL_SCANCODE_RSHIFT],
		key_state[SDL_SCANCODE_LCTRL]  || key_state[SDL_SCANCODE_RCTRL]) && button;

	if (key_pressed(SDL_SCANCODE_TAB))
		g_atmosphere_tweaks.show_tweaks = !g_atmosphere_tweaks.show_tweaks;

	int scroll_x = input_mouse_wheel_sum.wheel.x, scroll_y = input_mouse_wheel_sum.wheel.y;

	// SDL_SetRelativeMouseMode(relative_mouse);
	trackball_step(&lua_trackball, mouse_x, mouse_y, relative_mouse, scroll_x, scroll_y);
}

void lua_scene_render()
{
	/* Call Lua script render function */
	int top = lua_gettop(L);
	if (lua_getfield(L, LUA_REGISTRYINDEX, "tu_lua_scene") == LUA_TTABLE) {
		lua_getfield(L, -1, "render");
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			//TODO(Gavin): Check error object
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

	glUseProgram(SHADER);
	glClearColor(0.01f, 0.22f, 0.23f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//Compute inverse eye frame.
	amat4 combined_eye_frame = (amat4){lua_trackball.camera.a, eye_frame.t};
	amat4 inv_eye_frame;
	float proj_view_mat[16];
	float model_mat[16];
	float dir_mat[9];
	float model_view_proj_mat[16];
	{
		amat4 model_frame = {MAT3_IDENT, tri_frame.t};
		amat4_to_array(model_frame, model_mat);
		
		inv_eye_frame = amat4_inverse(combined_eye_frame);
		float tmp[16];
		amat4_to_array(inv_eye_frame, tmp);
		amat4_buf_mult(proj_mat, tmp, proj_view_mat);
	}
	amat4_buf_mult(proj_view_mat, model_mat, model_view_proj_mat);
	mat3_to_array_cm(combined_eye_frame.a, dir_mat);

	//Draw in wireframe if 'z' is held down.
	bool wireframe = key_state[SDL_SCANCODE_Z];
	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// glActiveTexture(GL_TEXTURE0);
	// glBindTexture(GL_TEXTURE_2D, test_gl_tx);
	// glUniform1i(SAMPLER0, 0);

	glUniformMatrix4fv(MM, 1, true, model_mat);
	glUniformMatrix4fv(MVPM, 1, true, model_view_proj_mat);
	glUniformMatrix3fv(DM, 1, false, dir_mat);
	glUniform3f(EP, VEC3_COORDS(combined_eye_frame.t));
	// glUniform3f(LP, VEC3_COORDS(light_pos));
	glUniform4f(PL, VEC3_COORDS(ico_frame.t), g_atmosphere_tweaks.planet_radius);
	glUniform1f(AH, g_atmosphere_tweaks.atmosphere_height);
	glUniform1f(FL, g_atmosphere_tweaks.focal_length);
	glUniform1f(AB, g_atmosphere_tweaks.samples_ab);
	glUniform1f(CP, g_atmosphere_tweaks.samples_cp);
	glUniform1f(TI, g_atmosphere_tweaks.scene_time);
	glUniform1f(PP, g_atmosphere_tweaks.p_and_p);
	glUniform1f(IS, (g_atmosphere_tweaks.planet_radius + g_atmosphere_tweaks.atmosphere_height) / ico_inscribed_radius(2*x));
	glUniform3fv(AIRB, 1, g_atmosphere_tweaks.air_b);
	glUniform1f(SH, g_atmosphere_tweaks.scale_height);
	glUniform1f(PS, g_atmosphere_tweaks.planet_scale);
	glUniform2f(RES, screen_width, screen_height);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glDrawArrays(GL_TRIANGLES, 0, LENGTH(ico_i));

	// glUniform1i(ROWS, rows);
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, get_shared_tri_tile_indices_buffer_object(rows));
	// checkErrors("After binding INBO");
	// glDrawElementsInstanced(GL_TRIANGLE_STRIP, num_tri_tile_indices(rows), GL_UNSIGNED_INT, NULL, 20);
	// checkErrors("After instanced draw");

	glBindVertexArray(0);

	if (g_atmosphere_tweaks.show_tweaks)
		meter_draw_all(&g_atmosphere_meters);
}
