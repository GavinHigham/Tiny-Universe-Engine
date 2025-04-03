#include "luaengine/lua_configuration.h"
#include "drawf.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "graphics.h"
#include "input_event.h"
#include "macros.h"
#include "math/utility.h"
#include "proctri_scene.h"
#include "scene.h"
#include "shader_utils.h"
#include "space/triangular_terrain_tile.h"

#include <assert.h>
#include <glla.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define USING_ROWS

/* Implementing scene "interface" */

SCENE_IMPLEMENT(proctri);

static float screen_width = 640, screen_height = 480;
static float mouse_x = 0, mouse_y = 0;

static amat4 eye_frame = {.a = MAT3_IDENT, .t = {0, 0, 5}}; 
static amat4 tri_frame = {.a = MAT3_IDENT, .t = {0, 0, 0}};
static float proj_mat[16];

#define NUM_ROWS 16
#define VERTS_PER_ROW(rows) ((rows+2)*(rows+1)/2)
static const int rows = NUM_ROWS;
extern int PRIMITIVE_RESTART_INDEX;

/* Lua Config */
extern lua_State *L;

/* OpenGL Variables */

static GLuint ROWS;
static GLuint SHADER, VAO, MM, MVPM, TEXSCALE, VBO, INBO;
static GLint SAMPLER0, VLERPS_ATTR;
static GLint POS_ATTR[3] = {1,2,3}, TX_ATTR[3] = {4,5,6};
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

//times three vertices, times 20 tiles.
static struct instance_attributes {
	float pos[9];
	float tx[6];
} instance_data[20];
//float uniform_data[tu_len * 3 * 20];

int proctri_scene_init(bool reload)
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	/* Shader initialization */
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("glsl330", "#version 330");

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "versions.glsl330", "common.noise.GL33", "proctri.vertex.GL33"),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "versions.glsl330", "common.noise.GL33", "proctri.fragment.GL33"),
	};
	SHADER = glsw_new_shader_program(shader, LENGTH(shader));
	glswShutdown();

	if (!SHADER) {
		proctri_scene_deinit();
		return -1;
	}

	/* Retrieve uniform variable handles */

	MM       = glGetUniformLocation(SHADER, "model_matrix");
	MVPM     = glGetUniformLocation(SHADER, "model_view_projection_matrix");
	SAMPLER0 = glGetUniformLocation(SHADER, "diffuse_tx");
	ROWS     = glGetUniformLocation(SHADER, "rows");
	TEXSCALE = glGetUniformLocation(SHADER, "tex_scale");
	checkErrors("After getting uniform handles");

	/* Vertex data */

	float tri_lerps[2 * VERTS_PER_ROW(NUM_ROWS)];
	get_tri_lerp_vals(tri_lerps, rows);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tri_lerps), tri_lerps, GL_STATIC_DRAW);

	glGenBuffers(1, &INBO);
	glBindBuffer(GL_ARRAY_BUFFER, INBO);
	checkErrors("After gen indexed array buffer");

	/* Vertex attributes */

	int attr_div = 1;
	for (int i = 0; i < 3; i++)
	{
		glEnableVertexAttribArray(POS_ATTR[i]);
		glVertexAttribPointer(POS_ATTR[i], 3, GL_FLOAT, GL_FALSE,
			sizeof(struct instance_attributes), (void *)(offsetof(struct instance_attributes, pos) + i*3*sizeof(float)));
		glVertexAttribDivisor(POS_ATTR[i], attr_div);
		checkErrors("After attr divisor for pos");
	}

	for (int i = 0; i < 3; i++)
	{
		glEnableVertexAttribArray(TX_ATTR[i]);
		glVertexAttribPointer(TX_ATTR[i], 2, GL_FLOAT, GL_FALSE,
			sizeof(struct instance_attributes), (void *)(offsetof(struct instance_attributes, tx) + i*2*sizeof(float)));
		glVertexAttribDivisor(TX_ATTR[i], attr_div);
		checkErrors("After attr divisor for attr");
	}

	VLERPS_ATTR = 7; //glGetAttribLocation(SHADER, "vlerp"); 
	glEnableVertexAttribArray(VLERPS_ATTR);

	checkErrors("After setting attrib divisors");
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	checkErrors("After binding VBO");
	glVertexAttribPointer(VLERPS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, 0);
	checkErrors("After setting VBO attrib pointer");

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
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);
	glBindVertexArray(0);

	/* For rotating the icosahedron */
	//SDL_SetRelativeMouseMode(true);

	char texture_path[gettmpglobstr(L, "proctri_tex", "grass.png", NULL)];
	                  gettmpglobstr(L, "proctri_tex", "grass.png", texture_path);
	test_gl_tx = load_gl_texture(texture_path);
	glUseProgram(SHADER);
	glUniform1f(TEXSCALE, getglob(L, "tex_scale", 1.0));
	glUseProgram(0);

	return 0;
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
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, get_shared_tri_tile_indices_buffer_object(rows));
	checkErrors("After binding INBO");
	glDrawElementsInstanced(GL_TRIANGLE_STRIP, num_tri_tile_indices(rows), GL_UNSIGNED_INT, NULL, 20);
	checkErrors("After instanced draw");

	glBindVertexArray(0);
}
