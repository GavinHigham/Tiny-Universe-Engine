#include "atmosphere_scene.h"
#include "scene.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "macros.h"
#include "math/utility.h"
#include "drawf.h"
#include "input_event.h"
#include "trackball/trackball.h"
#include "meter/meter.h"
#include "meter/meter_ogl_renderer.h"
#include "luaengine/lua_configuration.h"
#include "shader_utils.h"

#include <glla.h>
#include <stdio.h>
#include <math.h>
#include "graphics.h"

/* Lua Config */
extern lua_State *L;

/* Implementing scene "interface" */
SCENE_IMPLEMENT(atmosphere);
static struct trackball atmosphere_trackball;

static float screen_width = 640, screen_height = 480;
static float mouse_x = 0, mouse_y = 0;

static amat4 eye_frame = {.a = MAT3_IDENT, .t = {0, 0, 0}}; 
static amat4 ico_frame = {.a = MAT3_IDENT, .t = {0, 0, 0}};
static float proj_mat[16];

//Adapted from http://www.glprogramming.com/red/chapter02.html
static const float x = 0.525731112119133606;
static const float z = 0.850650808352039932;
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

/* OpenGL Variables */
static GLuint gl_buffers[2];
#define VBO gl_buffers[0]
#define IBO gl_buffers[1]
static GLuint SHADER, VAO, MM, DM, MVPM, EP, LP, PL, FL, IS, TI, AH, AB, PP, CP, RES, AIRB, SH, PS;
static GLint POSITION_ATTR, TX_ATTR;

/* Scene Variables */
struct atmosphere_tweaks g_atmosphere_tweaks;

/* UI */
meter_ctx g_atmosphere_meters;

struct atmosphere_tweaks atmosphere_load_tweaks(lua_State *L, const char *tweaks_table_name)
{
	if (lua_getglobal(L, tweaks_table_name) != LUA_TTABLE) {
		printf("Global with name %s is not a table!\n", tweaks_table_name);
		lua_newtable(L);
	}

	#define load_val(value_name, default_value) .value_name = getopttopfield(L, #value_name, default_value)
	struct atmosphere_tweaks at = {
		load_val(planet_radius, 1.0),
		load_val(fov, M_PI/6.0),
		load_val(atmosphere_height, 0.02),
		load_val(scene_time, 0.0),
		load_val(samples_ab, 10),
		load_val(samples_cp, 10),
		load_val(p_and_p, 1.0),
		load_val(scale_height, 8500.0),
		load_val(planet_scale, 6371000.0),
		load_val(show_tweaks, true),
		.air_b = {
			getopttopfield(L, "air_b_r", 0.00000519673),
			getopttopfield(L, "air_b_g", 0.0000121427),
			getopttopfield(L, "air_b_b", 0.0000296453),
		}
	};
	#undef load_val
	at.focal_length = fov_to_focal(at.fov);

	lua_settop(L, 0);
	return at;
}

void atmosphere_fov_updated(char *name, enum meter_state state, float value, void *context)
{
	struct atmosphere_tweaks *at = context;
	at->focal_length = fov_to_focal(value);
	make_projection_matrix(g_atmosphere_tweaks.fov, screen_width/screen_height, 0.1, 200, proj_mat);
}

void 
atmosphere_meters_init(lua_State *L, meter_ctx *M, struct atmosphere_tweaks *at, float screen_width, float screen_height)
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
			.callback = atmosphere_fov_updated, .callback_context = at,
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
		meter_add(M, w->name, w->style.width, w->style.height, w->min, *w->target, w->max);
		meter_target(M, w->name, w->target);
		meter_position(M, w->name, w->x, w->y + y_offset);
		meter_callback(M, w->name, w->callback, w->callback_context);
		meter_style(M, w->name, w->color.fill, w->color.border, w->color.font, w->style.padding, 0);
		meter_always_snap(M, w->name, w->always_snap);
		y_offset += 25;
	}
}

int atmosphere_scene_init(bool reload)
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(2, gl_buffers); /* VBO, IBO */

	/* Shader initialization */
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("GL33", "#version 330");

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "atmosphere.vertex.GL33"),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "atmosphere.fragment.GL33", "common.utility.GL33", "common.noise.GL33", "common.lighting.GL33", "common.debugging.GL33"),
	};
	SHADER = glsw_new_shader_program(shader, LENGTH(shader));
	glswShutdown();

	if (!SHADER)
		goto error;

	MM   = glGetUniformLocation(SHADER, "model_matrix");
	DM   = glGetUniformLocation(SHADER, "dir_mat");
	MVPM = glGetUniformLocation(SHADER, "model_view_projection_matrix");
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

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	/* Vertex attributes */

	POSITION_ATTR = glGetAttribLocation(SHADER, "position"); 
	TX_ATTR       = glGetAttribLocation(SHADER, "tx");
	if (POSITION_ATTR == -1 || TX_ATTR == -1) {
		printf("Vertex attributes were not retrieved successfully.\n");
		goto error;
	}

	glEnableVertexAttribArray(POSITION_ATTR);
	glEnableVertexAttribArray(TX_ATTR);
	glVertexAttribPointer(POSITION_ATTR, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, x));
	glVertexAttribPointer(TX_ATTR,       2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, u));

	/* Index buffer */

	// Turned off because I needed duplicate all vertices for texture seams, so indexing shouldn't be faster.
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	// glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ico_i), ico_i, GL_STATIC_DRAW);

	/* Misc. OpenGL bits */
	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glBindVertexArray(0);

	/* For rotating the icosahedron */
	//SDL_SetRelativeMouseMode(true);
	atmosphere_trackball = trackball_new(ico_frame.t, 5.0);
	trackball_set_speed(&atmosphere_trackball, 1.0/200.0, 1.0/200.0, 1/10.0);
	trackball_set_bounds(&atmosphere_trackball, INFINITY, INFINITY, INFINITY, INFINITY);

	g_atmosphere_tweaks = atmosphere_load_tweaks(L, "atmosphere_defaults");
	atmosphere_meters_init(L, &g_atmosphere_meters, &g_atmosphere_tweaks, screen_width, screen_height);

	return 0;

error:
	atmosphere_scene_deinit();
	return -1;
}

void atmosphere_scene_resize(float width, float height)
{
	glViewport(0, 0, width, height);
	screen_width = width;
	screen_height = height;
	meter_resize_screen(&g_atmosphere_meters, screen_width, screen_height);
	make_projection_matrix(g_atmosphere_tweaks.fov, screen_width/screen_height, 0.1, 200, proj_mat);
}

void atmosphere_scene_deinit()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(2, &gl_buffers[VBO]);
	glDeleteProgram(SHADER);
}

void atmosphere_scene_update(float dt)
{
	eye_frame.t += mat3_multvec(atmosphere_trackball.camera.a, (vec3){
		key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A],
		key_state[SDL_SCANCODE_E] - key_state[SDL_SCANCODE_Q],
		key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W],
	} * 0.15);

	g_atmosphere_tweaks.scene_time += dt;
	
	Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
	bool button = buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT);

	if (g_atmosphere_tweaks.show_tweaks)
		button = !meter_mouse_relative(&g_atmosphere_meters, mouse_x, mouse_y, button,
		key_state[SDL_SCANCODE_LSHIFT] || key_state[SDL_SCANCODE_RSHIFT],
		key_state[SDL_SCANCODE_LCTRL]  || key_state[SDL_SCANCODE_RCTRL]) && button;

	if (key_pressed(SDL_SCANCODE_TAB))
		g_atmosphere_tweaks.show_tweaks = !g_atmosphere_tweaks.show_tweaks;

	int scroll_x = input_mouse_wheel_sum.wheel.x, scroll_y = input_mouse_wheel_sum.wheel.y;
	trackball_step(&atmosphere_trackball, mouse_x, mouse_y, button, scroll_x, scroll_y);
}

void atmosphere_scene_render()
{
	glBindVertexArray(VAO);
	glUseProgram(SHADER);
	// glClearColor(0.01f, 0.22f, 0.23f, 1.0f);
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//Compute inverse eye frame.
	amat4 combined_eye_frame = (amat4){atmosphere_trackball.camera.a, atmosphere_trackball.camera.t + eye_frame.t};
	amat4 inv_eye_frame;
	float proj_view_mat[16];
	float model_mat[16];
	float dir_mat[9];
	float model_view_proj_mat[16];
	{
		amat4 model_frame = {MAT3_IDENT, ico_frame.t};
		amat4_to_array(model_frame, model_mat);
		
		//inv_eye_frame = amat4_inverse(eye_frame);
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

	glBindVertexArray(0);

	if (g_atmosphere_tweaks.show_tweaks)
		meter_draw_all(&g_atmosphere_meters);
}
