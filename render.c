#include <math.h>
#include <assert.h>
#include <string.h>
#include <GL/glew.h>
#include "render.h"
#include "init.h"
#include "shaders.h"
#include "shader_program.h"
#include "keyboard.h"
#include "default_settings.h"
#include "affine_matrix4.h"
#include "matrix3.h"
#include "vector3.h"

#define FOV M_PI/2.0
#define PRIMITIVE_RESTART_INDEX 0xFFFF

struct render_context {
	GLuint vbo, cbo, nbo, ibo;
};

#include "models.h"

static struct render_context current_context;
static MAT3 MVM_r = MAT3_IDENT;
static V3 MVM_t = {{0, 0, -8}};
static float light_time = 0;
GLfloat distance = -8.0;

int buffer_normal_cube();

//Set up everything needed to start rendering frames.
void init_render()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_PRIMITIVE_RESTART);
	glClearDepth(0.0);
	glDepthFunc(GL_GREATER);
	glClearColor(0.0f, 0.0f, 0.25f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glGenBuffers(1, &current_context.vbo);
	glGenBuffers(1, &current_context.cbo);
	glGenBuffers(1, &current_context.nbo);
	glGenBuffers(1, &current_context.ibo);
}

//Buffer all the data needed to render a cube with correct normals, and some colors.
int buffer_normal_cube(struct render_context rc)
{
	GLfloat vertices[] = {
		//Top face
		 1.0,  1.0, -1.0,
		 1.0,  1.0,  1.0,
		-1.0,  1.0,  1.0,
		-1.0,  1.0, -1.0,
		//Back face
		 1.0,  1.0, -1.0,
		-1.0,  1.0, -1.0,
		-1.0, -1.0, -1.0,
		 1.0, -1.0, -1.0,
		//Right face
		 1.0,  1.0, -1.0,
		 1.0, -1.0, -1.0,
		 1.0, -1.0,  1.0,
		 1.0,  1.0,  1.0,
		//Bottom face
		-1.0, -1.0,  1.0,
		-1.0, -1.0, -1.0,
		 1.0, -1.0, -1.0,
		 1.0, -1.0,  1.0,
		//Front face
		-1.0, -1.0,  1.0,
		 1.0, -1.0,  1.0,
		 1.0,  1.0,  1.0,
		-1.0,  1.0,  1.0,
		//Left face
		-1.0, -1.0,  1.0,
		-1.0,  1.0,  1.0,
		-1.0,  1.0, -1.0,
		-1.0, -1.0, -1.0,
	};
	GLfloat colors[] = {
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		1.0, 0.0, 0.0,

		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 1.0, 0.0,

		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,
		0.0, 0.0, 1.0,

		1.0, 1.0, 0.0,
		1.0, 1.0, 0.0,
		1.0, 1.0, 0.0,
		1.0, 1.0, 0.0,

		0.0, 1.0, 1.0,
		0.0, 1.0, 1.0,
		0.0, 1.0, 1.0,
		0.0, 1.0, 1.0,

		1.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 0.0, 1.0,
		1.0, 0.0, 1.0
	};
	GLfloat normals[] = {
		//Top face
		 0.0,  1.0,  0.0,
		 0.0,  1.0,  0.0,
		 0.0,  1.0,  0.0,
		 0.0,  1.0,  0.0,
		//Back face
		 0.0,  0.0, -1.0,
		 0.0,  0.0, -1.0,
		 0.0,  0.0, -1.0,
		 0.0,  0.0, -1.0,
		//Right face
		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,
		//Bottom face
 		 0.0, -1.0,  0.0,
 		 0.0, -1.0,  0.0,
 		 0.0, -1.0,  0.0,
 		 0.0, -1.0,  0.0,
 		//Front face
		 0.0,  0.0,  1.0,
		 0.0,  0.0,  1.0,
		 0.0,  0.0,  1.0,
		 0.0,  0.0,  1.0,
		//Left face
		-1.0,  0.0,  0.0,
		-1.0,  0.0,  0.0,
		-1.0,  0.0,  0.0,
		-1.0,  0.0,  0.0,
	};
	GLuint indices[] = {
		0, 3, 2, 1,
		PRIMITIVE_RESTART_INDEX,
		4, 7, 6, 5,
		PRIMITIVE_RESTART_INDEX,
		8, 11, 10, 9,
		PRIMITIVE_RESTART_INDEX,
		12, 13, 14, 15,
		PRIMITIVE_RESTART_INDEX,
		16, 17, 18, 19,
		PRIMITIVE_RESTART_INDEX,
		20, 21, 22, 23
	};
	glBindBuffer(GL_ARRAY_BUFFER, rc.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, rc.cbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, rc.nbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rc.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	return sizeof(indices)/sizeof(indices[0]);
}

//Create a projection matrix with "fov" field of view, "a" aspect ratio, n and f near and far planes.
//Stick it into buffer buf, ready to send to OpenGL.
void make_projection_matrix(GLfloat fov, GLfloat a, GLfloat n, GLfloat f, GLfloat *buf, int buf_len)
{
	assert(buf_len == 16);
	GLfloat nn = 1.0/tan(fov/2.0);
	GLfloat tmp[] = {
		nn, 0,           0,              0,
		0, nn,           0,              0,
		0,  0, (f+n)/(f-n), (-2*f*n)/(f-n),
		0,  0,          -1,              1
	};
	assert(buf_len == sizeof(tmp)/sizeof(GLfloat));
	if (a >= 0)
		tmp[0] /= a;
	else
		tmp[5] *= a;
	memcpy(buf, tmp, sizeof(tmp));
}

void draw_cube(struct render_context rc, int index_count, GLfloat x, GLfloat y, GLfloat z)
{	
	GLfloat mvm_buf[16];

	mat3_v3_to_array(mvm_buf, sizeof(mvm_buf)/sizeof(mvm_buf[0]), MVM_r, MVM_t);
	glUniformMatrix4fv(simple_program.unif[MVM], 1, GL_TRUE, mvm_buf);
	mat3_v3_to_array(mvm_buf, sizeof(mvm_buf)/sizeof(mvm_buf[0]), mat3_transp(MVM_r), (V3){{0, 0, 0}});
	glUniformMatrix4fv(simple_program.unif[NMVM], 1, GL_TRUE, mvm_buf);
	glUniform3f(simple_program.unif[sun_light], 10*cos(light_time), 0, 10*sin(light_time) - distance);
	//glUniform3f(simple_program.unif[sun_light], 10, 10, 10);

	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX); //Used to draw two triangle fans with one draw call.
	//vertex_pos
	glEnableVertexAttribArray(simple_program.attr[vPos]);
	glBindBuffer(GL_ARRAY_BUFFER, rc.vbo);
	glVertexAttribPointer(simple_program.attr[vPos], 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//vColor
	glEnableVertexAttribArray(simple_program.attr[vColor]);
	glBindBuffer(GL_ARRAY_BUFFER, rc.cbo);
	glVertexAttribPointer(simple_program.attr[vColor], 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//vNormal
	glEnableVertexAttribArray(simple_program.attr[vNormal]);
	glBindBuffer(GL_ARRAY_BUFFER, rc.nbo);
	glVertexAttribPointer(simple_program.attr[vNormal], 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rc.ibo);
	//glDrawElements(GL_TRIANGLE_FAN, index_count, GL_UNSIGNED_INT, NULL);
	glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, NULL);
	checkErrors("After glDrawElements");
	//Clean up?
	glDisableVertexAttribArray(simple_program.attr[vPos]);
	glDisableVertexAttribArray(simple_program.attr[vColor]);
}

void render()
{
	GLfloat proj_mat[16];

	glUseProgram(simple_program.handle);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	make_projection_matrix(FOV, (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, -1, -10, proj_mat, sizeof(proj_mat)/sizeof(proj_mat[0]));
	glUniformMatrix4fv(simple_program.unif[projection_matrix], 1, GL_TRUE, proj_mat);

	//int num_indices = buffer_normal_cube(current_context);
	int num_indices = buffer_newship(current_context);
	draw_cube(current_context, num_indices, 0, 0, distance);
	// for (int i = 0; i < 10; i++)
	// 	for (int j = 0; j < 10; j++)
	// 		draw_cube(current_context, num_indices, 5*(i-5), 5*(j-5), distance);

	glUseProgram(0);
}

void update(float dt)
{
	light_time += dt;
	static float rotx, roty;
	if (keys[KEY_LEFT])
		roty = dt;
	if (keys[KEY_RIGHT])
		roty = -dt;
	if (keys[KEY_UP])
		rotx = dt;
	if (keys[KEY_DOWN])
		rotx = -dt;
	if (keys[KEY_EQUALS]) {
		MVM_t.A[2] += dt;
	}
	if (keys[KEY_MINUS]) {
		MVM_t.A[2] -= dt;
	}
	if (rotx != 0) {
		MVM_r = mat3_rot(MVM_r, 1, 0, 0, -rotx);
	}
	if (roty != 0) {
		MVM_r = mat3_rot(MVM_r, 0, 1, 0, -roty);
	}

	rotx = 0;
	roty = 0;
}
