#include "icosphere_scene.h"
#include "../scene.h"
#include "../glsw/glsw.h"
#include "../glsw_shaders.h"
#include "../macros.h"
#include "../math/utility.h"
#include "../drawf.h"
#include "../input_event.h"
#include "../trackball/trackball.h"

#include <glla.h>
#include <GL/glew.h>
#include <stdio.h>
#include <SDL2_image/SDL_image.h>

/* Implementing scene "interface" */
SCENE_IMPLEMENT(icosphere);
static GLuint load_gl_texture(char *path);
static struct trackball icosphere_trackball;

static SDL_Surface *test_surface = NULL;
static float screen_width = 640, screen_height = 480;
static int mouse_x = 0, mouse_y = 0;

static amat4 eye_frame = {.a = MAT3_IDENT, .t = {0, 0, 5}}; 
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
GLuint gl_buffers[2];
#define VBO gl_buffers[0]
#define IBO gl_buffers[1]
static GLuint SHADER, VAO, MM, MVPM;
static GLint POSITION_ATTR, TX_ATTR, SAMPLER0;
static GLuint test_gl_tx = 0;

int icosphere_scene_init()
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(2, gl_buffers); /* VBO, IBO */

	/* Shader initialization */
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("GL33", "#version 330");

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "flat_textured.vertex.GL33"),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "flat_textured.fragment.GL33"),
	};
	SHADER = glsw_new_shader_program(shader, LENGTH(shader));
	glswShutdown();

	if (!SHADER)
		goto error;

	MM       = glGetUniformLocation(SHADER, "model_matrix");
	MVPM     = glGetUniformLocation(SHADER, "model_view_projection_matrix");
	SAMPLER0 = glGetUniformLocation(SHADER, "diffuse_tx");

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
	icosphere_trackball = trackball_new(ico_frame.t, 3);
	trackball_set_speed(&icosphere_trackball, 1.0/50.0, 1.0/200.0);
	trackball_set_bounds(&icosphere_trackball, M_PI / 3.0, M_PI / 3.0, INFINITY, INFINITY);

	test_gl_tx = load_gl_texture("grass.png");

	return 0;

error:
	icosphere_scene_deinit();
	return -1;
}

void icosphere_scene_resize(float width, float height)
{
	glViewport(0, 0, width, height);
	screen_width = width;
	screen_height = height;
	make_projection_matrix(65, screen_width/screen_height, 0.1, 200, proj_mat);	
}

void icosphere_scene_deinit()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(2, &gl_buffers[VBO]);
	glDeleteProgram(SHADER);
	SDL_FreeSurface(test_surface);
}

void icosphere_scene_update(float dt)
{
	eye_frame.t += (vec3){
		key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A],
		key_state[SDL_SCANCODE_E] - key_state[SDL_SCANCODE_Q],
		key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W],
	} * 0.15;
	
	Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
	trackball_step(&icosphere_trackball, mouse_x, mouse_y, buttons & SDL_BUTTON(SDL_BUTTON_LEFT));
}

void icosphere_scene_render()
{
	glBindVertexArray(VAO);
	glUseProgram(SHADER);
	glClearColor(0.01f, 0.22f, 0.23f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//Compute inverse eye frame.
	amat4 inv_eye_frame;
	float proj_view_mat[16];
	float model_mat[16];
	float model_view_proj_mat[16];
	{
		float s1 = sin(4*mouse_y/screen_height);
		float c1 = cos(4*mouse_y/screen_height);
		float s2 = sin(4*mouse_x/screen_width);
		float c2 = cos(4*mouse_x/screen_width);
		mat3 xrot = mat3_rotmat(1, 0, 0, s1, c1);
		mat3 yrot = mat3_rotmat(0, 1, 0, s2, c2);
		//amat4 rot_model_frame = {mat3_mult(mat3_mult(ico_frame.a, yrot), xrot), ico_frame.t};
		amat4 rot_model_frame = {MAT3_IDENT, ico_frame.t};
		amat4_to_array(rot_model_frame, model_mat);
		
		//inv_eye_frame = amat4_inverse(eye_frame);
		inv_eye_frame = amat4_inverse(icosphere_trackball.camera);
		float tmp[16];
		amat4_to_array(inv_eye_frame, tmp);
		amat4_buf_mult(proj_mat, tmp, proj_view_mat);
	}
	amat4_buf_mult(proj_view_mat, model_mat, model_view_proj_mat);

	//Draw in wireframe if 'z' is held down.
	bool wireframe = key_state[SDL_SCANCODE_Z];
	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, test_gl_tx);
	glUniform1i(SAMPLER0, 0);

	glUniformMatrix4fv(MM, 1, true, model_mat);
	glUniformMatrix4fv(MVPM, 1, true, model_view_proj_mat);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glDrawArrays(GL_TRIANGLES, 0, LENGTH(ico_i));

	glBindVertexArray(0);
}

static GLuint load_gl_texture(char *path)
{
	GLuint texture = 0;
	SDL_Surface *surface = IMG_Load(path);
	GLenum texture_format;
	if (!surface) {
		printf("Texture %s could not be loaded.\n", path);
		return 0;
	}

	// Get the number of channels in the SDL surface.
	int num_colors = surface->format->BytesPerPixel;
	bool rgb = surface->format->Rmask == 0x000000ff;
	if (num_colors == 4) {
		texture_format = rgb ? GL_RGBA : GL_BGRA;
	} else if (num_colors == 3) {
		texture_format = rgb ? GL_RGB : GL_BGR;
	} else {
		printf("Image does not have at least 3 color channels.\n");
		goto error;
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SDL_LockSurface(surface);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, surface->w, surface->h, 0, texture_format, GL_UNSIGNED_BYTE, surface->pixels);
	SDL_UnlockSurface(surface);

error:
	SDL_FreeSurface(surface);
	return texture;
}
