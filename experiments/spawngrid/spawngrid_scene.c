#include "spawngrid_scene.h"
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
SCENE_IMPLEMENT(spawngrid);

static float screen_width = 640, screen_height = 480;
static int mouse_x = 0, mouse_y = 0;

static GLfloat vertices[] = {-1, -1, 1, -1, -1, 1, 1, 1};

/* OpenGL Variables */
static GLuint gl_buffers[2];
#define VBO gl_buffers[0]
#define IBO gl_buffers[1]
static GLuint SHADER, VAO, TI, PP, RES, MOU;
static GLint POSITION_ATTR;

/* Scene Variables */
struct spawngrid_tweaks {
	float scene_time;
	float p_and_p;
	bool show_tweaks;
} g_spawngrid_tweaks;

/* UI */
meter_ctx g_spawngrid_meters;

struct spawngrid_tweaks spawngrid_load_tweaks(lua_State *L, const char *tweaks_table_name)
{
	if (lua_getglobal(L, tweaks_table_name) != LUA_TTABLE) {
		printf("Global with name %s is not a table!\n", tweaks_table_name);
		lua_newtable(L);
	}

	#define load_val(value_name, default_value) .value_name = getopttopfield(L, #value_name, default_value)
	struct spawngrid_tweaks at = {
		load_val(scene_time, 0.0),
		load_val(p_and_p, 0.2),
		load_val(show_tweaks, true),
	};
	#undef load_val

	lua_settop(L, 0);
	return at;
}

void spawngrid_meters_init(lua_State *L, meter_ctx *M, struct spawngrid_tweaks *at, float screen_width, float screen_height)
{
	/* Set up meter module */
	float y_offset = 25.0;
	meter_init(M, screen_width, screen_height, 20, meter_ogl_renderer);
	struct widget_meter_style wstyles = {.width = 600, .height = 20, .padding = 2.0};
	widget_meter widgets[] = {
		{
			.name = "Poke and Prod Variable", .x = 5.0, .y = 0, .min = -30, .max = 30,
			.target = &at->p_and_p,
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

int spawngrid_scene_init(bool reload)
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(2, gl_buffers); /* VBO, IBO */

	/* Shader initialization */
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("GL33", "#version 330");

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "spawngrid.vertex.GL33"),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "spawngrid.fragment.GL33", "common.utility.GL33", "common.noise.GL33", "common.lighting.GL33", "common.debugging.GL33"),
	};
	SHADER = glsw_new_shader_program(shader, LENGTH(shader));
	glswShutdown();

	if (!SHADER)
		goto error;

	PP   = glGetUniformLocation(SHADER, "p_and_p");
	TI   = glGetUniformLocation(SHADER, "time");
	RES  = glGetUniformLocation(SHADER, "resolution");
	MOU  = glGetUniformLocation(SHADER, "mouse");

	/* Vertex data */

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	/* Vertex attributes */

	POSITION_ATTR = glGetAttribLocation(SHADER, "pos"); 
	if (POSITION_ATTR == -1) {
		printf("Vertex attributes were not retrieved successfully.\n");
		goto error;
	}

	glEnableVertexAttribArray(POSITION_ATTR);
	glVertexAttribPointer(POSITION_ATTR, 2, GL_FLOAT, GL_FALSE, 0, 0);
	checkErrors("After pos attr");

	/* Index buffer */

	//Not doing for this

	/* Misc. OpenGL bits */
	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glBindVertexArray(0);

	g_spawngrid_tweaks = spawngrid_load_tweaks(L, "spawngrid_defaults");
	spawngrid_meters_init(L, &g_spawngrid_meters, &g_spawngrid_tweaks, screen_width, screen_height);

	return 0;

error:
	spawngrid_scene_deinit();
	return -1;
}

void spawngrid_scene_resize(float width, float height)
{
	glViewport(0, 0, width, height);
	screen_width = width;
	screen_height = height;
	meter_resize_screen(&g_spawngrid_meters, screen_width, screen_height);
}

void spawngrid_scene_deinit()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(2, &gl_buffers[VBO]);
	glDeleteProgram(SHADER);
}

void spawngrid_scene_update(float dt)
{
	g_spawngrid_tweaks.scene_time += dt;
	
	Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
	bool button = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);

	if (g_spawngrid_tweaks.show_tweaks)
		button = !meter_mouse_relative(&g_spawngrid_meters, mouse_x, mouse_y, button,
		key_state[SDL_SCANCODE_LSHIFT] || key_state[SDL_SCANCODE_RSHIFT],
		key_state[SDL_SCANCODE_LCTRL]  || key_state[SDL_SCANCODE_RCTRL]) && button;

	if (key_pressed(SDL_SCANCODE_TAB))
		g_spawngrid_tweaks.show_tweaks = !g_spawngrid_tweaks.show_tweaks;
}

void spawngrid_scene_render()
{
	glBindVertexArray(VAO);
	glUseProgram(SHADER);
	// glClearColor(0.01f, 0.22f, 0.23f, 1.0f);
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//Draw in wireframe if 'z' is held down.
	bool wireframe = key_state[SDL_SCANCODE_Z];
	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glUniform1f(TI, g_spawngrid_tweaks.scene_time);
	glUniform1f(PP, g_spawngrid_tweaks.p_and_p);
	glUniform2f(RES, screen_width, screen_height);
	glUniform2f(MOU, mouse_x, mouse_y);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);

	if (g_spawngrid_tweaks.show_tweaks)
		meter_draw_all(&g_spawngrid_meters);
}
