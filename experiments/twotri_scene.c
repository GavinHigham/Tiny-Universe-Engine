#include "twotri_scene.h"
#include "../scene.h"
#include "../glsw/glsw.h"
#include "../glsw_shaders.h"
#include "../macros.h"
#include "../math/utility.h"
#include "../drawf.h"
#include "../math/utility.h"
#include "../input_event.h"
//#include "../space/triangular_terrain_tile.h"
#include "../configuration/lua_configuration.h"
#include "../trackball/trackball.h"

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

/* Lua Config */
extern lua_State *L;

/* OpenGL Variables */

static GLint POS_ATTR = 1;
static GLuint SHADER, VAO, VBO, RESOLUTION_UNIF, MOUSE_UNIF, TIME_UNIF, FOCAL_UNIF, VIEW_UNIF;
GLfloat vertices[] = {-1, -1, 1, -1, -1, 1, 1, 1};

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
	};
	SHADER = glsw_new_shader_program(shader, LENGTH(shader));
	glswShutdown();

	if (!SHADER) {
		twotri_scene_deinit();
		return -1;
	}

	/* Retrieve uniform variable handles */

	RESOLUTION_UNIF = glGetUniformLocation(SHADER, "iResolution");
	MOUSE_UNIF      = glGetUniformLocation(SHADER, "iMouse");
	TIME_UNIF       = glGetUniformLocation(SHADER, "iTime");
	FOCAL_UNIF      = glGetUniformLocation(SHADER, "iFocalLength");
	VIEW_UNIF        = glGetUniformLocation(SHADER, "view_mat");
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

	/* Misc. OpenGL bits */

	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glBindVertexArray(0);

	glUseProgram(0);

	/* Set up trackball */
	twotri_trackball = trackball_new((vec3){0, 0, -25}, 10);
	trackball_set_speed(&twotri_trackball, 1.0/50.0, 1.0/200.0);
	trackball_set_bounds(&twotri_trackball, M_PI / 3.0, M_PI / 3.0, INFINITY, INFINITY);

	return 0;
}

void twotri_scene_resize(float width, float height)
{
	glViewport(0, 0, width, height);
	screen_width = width;
	screen_height = height;
	//make_projection_matrix(65, screen_width/screen_height, 0.1, 200, proj_mat);	
}

void twotri_scene_deinit()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(SHADER);
}

void twotri_scene_update(float dt)
{
	// eye_frame.t += (vec3){
	// 	key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A],
	// 	key_state[SDL_SCANCODE_E] - key_state[SDL_SCANCODE_Q],
	// 	key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W],
	// } * 0.15;
	
	Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
	trackball_step(&twotri_trackball, mouse_x, mouse_y, buttons & SDL_BUTTON(SDL_BUTTON_LEFT));
}

void twotri_scene_render()
{
	glBindVertexArray(VAO);
	glUseProgram(SHADER);
	glClearColor(0.01f, 0.22f, 0.23f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	{
		float view_mat[16];
		amat4 inv_eye_frame = amat4_inverse(twotri_trackball.camera);
		// amat4_to_array(inv_eye_frame, view_mat);
		// for (int i = 0; i < 16; i++)
		// 	printf("%f, ", view_mat[i]);
		// printf("\n");
		glUniformMatrix4fv(VIEW_UNIF, 1, true, view_mat);
	}

	glUniform1f(TIME_UNIF, SDL_GetTicks() / 1000.0);
	glUniform2f(RESOLUTION_UNIF, screen_width, screen_height);
	glUniform4f(MOUSE_UNIF, mouse_x, mouse_y, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	checkErrors("After draw");

	glBindVertexArray(0);
}
