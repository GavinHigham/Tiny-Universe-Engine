#include "proctri_scene.h"
#include "../scene.h"
#include "../glsw/glsw.h"
#include "../glsw_shaders.h"
#include "../macros.h"
#include "../math/utility.h"
#include "../drawf.h"
#include "../gl_utils.h"
#include "../input_event.h"
#include "../space/triangular_terrain_tile.h"

#include <glla.h>
#include <GL/glew.h>
#include <stdio.h>
#include <assert.h>
#include <SDL2_image/SDL_image.h>

#define USING_ROWS

/* Implementing scene "interface" */

SCENE_IMPLEMENT(proctri);
static GLuint load_gl_texture(char *path);
int tri_lerp_vals(float *lerps, int num_rows);

static SDL_Surface *test_surface = NULL;
static float screen_width = 640, screen_height = 480;
static int mouse_x = 0, mouse_y = 0;


static amat4 eye_frame = {.a = MAT3_IDENT, .t = {0, 0, 5}}; 
static amat4 tri_frame = {.a = MAT3_IDENT, .t = {0, 0, 0}};
static float proj_mat[16];
float positions[] = {0.5,0,0, -.5,1,0, 0.5,0,1};
float tx_coords[] = {0.5,0, 0,1, 1,1};

#define NUM_ROWS 128
#define VERTS_PER_ROW(rows) ((rows+2)*(rows+1)/2)
static const int rows = NUM_ROWS;
extern int PRIMITIVE_RESTART_INDEX;

/* OpenGL Variables */

static GLuint ROWS;
static GLuint SHADER, VAO, VBO, MM, MVPM, VPOS, VTX;
static GLint SAMPLER0, VLERPS_ATTR;
static GLuint test_gl_tx = 0;

//Adapted from http://www.glprogramming.com/red/chapter02.html
static const float x = 0.525731112119133606;
static const float z = 0.850650808352039932;
static const float ico_v[] = {    
	-x,  0, z,  x,  0,  z, -x, 0, -z,  x, 0, -z, 0,  z, x,  0,  z, -x,
	 0, -z, x,  0, -z, -x,  z, x,  0, -z, x,  0, z, -x, 0, -z, -x,  0
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

int proctri_scene_init()
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	/* Shader initialization */
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("GL33", "#version 330");

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "experimental.vertex.GL33"),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "experimental.fragment.GL33"),
	};
	SHADER = glsw_new_shader_program(shader, LENGTH(shader));
	glswShutdown();

	if (!SHADER)
		goto error;

	MM       = glGetUniformLocation(SHADER, "model_matrix");
	MVPM     = glGetUniformLocation(SHADER, "model_view_projection_matrix");
	SAMPLER0 = glGetUniformLocation(SHADER, "diffuse_tx");
	VPOS     = glGetUniformLocation(SHADER, "vpos");
	VTX      = glGetUniformLocation(SHADER, "vtx");
	ROWS     = glGetUniformLocation(SHADER, "rows");

	/* Vertex data */

	float tri_lerps[2 * VERTS_PER_ROW(NUM_ROWS)];
	tri_lerp_vals(tri_lerps, rows);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tri_lerps), tri_lerps, GL_STATIC_DRAW);

	/* Vertex attributes */

	VLERPS_ATTR = glGetAttribLocation(SHADER, "vlerp"); 
	if (VLERPS_ATTR == -1) {
		printf("Vertex attributes were not retrieved successfully.\n");
		goto error;
	}

	glEnableVertexAttribArray(VLERPS_ATTR);
	glVertexAttribPointer(VLERPS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, 0);

	/* Misc. OpenGL bits */

	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);
	glBindVertexArray(0);

	/* For rotating the icosahedron */
	SDL_SetRelativeMouseMode(true);

	test_gl_tx = load_gl_texture("pizza.png");

	return 0;

error:
	proctri_scene_deinit();
	return -1;
}

void proctri_scene_resize(float width, float height)
{
	glViewport(0, 0, width, height);
	screen_width = width;
	screen_height = height;
	make_projection_matrix(65, screen_width/screen_height, 0.1, 200, proj_mat);	
}

void proctri_scene_deinit()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(SHADER);
	SDL_FreeSurface(test_surface);
}

void proctri_scene_update(float dt)
{
	eye_frame.t += (vec3){
		key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A],
		key_state[SDL_SCANCODE_E] - key_state[SDL_SCANCODE_Q],
		key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W],
	} * 0.15;
	
	SDL_GetMouseState(&mouse_x, &mouse_y);
}

void proctri_scene_render()
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
		amat4 rot_model_frame = {mat3_mult(mat3_mult(tri_frame.a, yrot), xrot), tri_frame.t};
		amat4_to_array(rot_model_frame, model_mat);
		
		inv_eye_frame = amat4_inverse(eye_frame);
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
	glUniform1i(ROWS, rows);
	//glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, get_shared_tri_tile_indices_buffer_object(rows));

	for (int i = 0; i < 20; i++) {
		float pos[9] = {x,  0,  z, -x,  0, z, 0,  z, x};
		float tx[6] = {0};
		for (int j = 0; j < 3; j++)
			memcpy(pos + j*3, &ico_v[ico_i[i*3 + j]*3], 3*sizeof(float));
		memcpy(tx, &ico_tx[i*6], 6*sizeof(float));
		glUniform3fv(VPOS, 3, pos);
		glUniform2fv(VTX, 3, tx);
		glDrawElements(GL_TRIANGLE_STRIP, num_tri_tile_indices(rows), GL_UNSIGNED_INT, NULL);
	}

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

int tri_lerp_vals(float *lerps, int num_rows)
{
	int written = 0;
	//Avoid divide-by-zero for 0th row.
	lerps[written++] = 0;
	lerps[written++] = 0;
	//Row by row, from top to bottom.
	for (int i = 1; i <= num_rows; i++) {
		float left = (float)i/num_rows;
		//Along each row, from left to right.
		for (int j = 0; j <= i; j++) {
			float right = (float)j/i;
			lerps[written++] = right;
			lerps[written++] = left;
		}
	}
	return written;
}
