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

#define FOV M_PI/2.0
#define PRIMITIVE_RESTART_INDEX 0xFFFF

struct render_context {
	GLuint vbo, cbo, nbo, ibo;
};
static struct render_context current_context;
static GLfloat mvm_buf[16];
static float light_time = 0;
GLfloat distance = -3.0;

void buffer_cube();
void buffer_normal_cube();

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

void buffer_cube(struct render_context rc) {
	GLfloat vertices[] = {
		 1.0,  1.0, -1.0,
		 1.0,  1.0,  1.0,
		-1.0,  1.0,  1.0,
		-1.0,  1.0, -1.0,
		-1.0, -1.0, -1.0,
		-1.0, -1.0,  1.0,
		 1.0, -1.0,  1.0,
		 1.0, -1.0, -1.0
	};
	GLfloat colors[] = {
		0.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		1.0, 1.0, 0.0,
		0.0, 1.0, 1.0,
		1.0, 0.0, 1.0,
		0.0, 0.0, 0.0,
		1.0, 1.0, 1.0
	};
	GLuint indices[] = {
		0, 1, 6, 7, 4, 3, 2, 1, //Need to stop and draw a second fan here.
		PRIMITIVE_RESTART_INDEX, //Primitive restart index.
		5, 1, 2, 3, 4, 7, 6, 1
	};
	glBindBuffer(GL_ARRAY_BUFFER, rc.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, rc.cbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rc.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void buffer_normal_cube(struct render_context rc)
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
		 0.0,  1.0,  0.0,
		 0.0,  1.0,  0.0,
		 0.0,  1.0,  0.0,
		 0.0,  1.0,  0.0,

		 0.0,  0.0, -1.0,
		 0.0,  0.0, -1.0,
		 0.0,  0.0, -1.0,
		 0.0,  0.0, -1.0,

		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,

 		 0.0, -1.0,  0.0,
 		 0.0, -1.0,  0.0,
 		 0.0, -1.0,  0.0,
 		 0.0, -1.0,  0.0,

		 0.0,  0.0,  1.0,
		 0.0,  0.0,  1.0,
		 0.0,  0.0,  1.0,
		 0.0,  0.0,  1.0,

		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,
		 1.0,  0.0,  0.0,
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
}

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

void draw_cube(struct render_context rc)
{	
	glUniform3f(simple_program.unif[sun_light], 7*cos(light_time), 7*sin(light_time), 10);

	buffer_normal_cube(current_context);

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
	glDrawElements(GL_TRIANGLE_FAN, 29, GL_UNSIGNED_INT, NULL);
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
	glUniformMatrix4fv(simple_program.unif[model_view_matrix], 1, GL_TRUE, mvm_buf);

	draw_cube(current_context);

	glUseProgram(0);
}

void update(float dt)
{
	light_time += dt * 3;
	static float rotx, roty;
	if (keys[KEY_LEFT])
		roty += dt;
	if (keys[KEY_RIGHT])
		roty -= dt;
	if (keys[KEY_UP])
		rotx += dt;
	if (keys[KEY_DOWN])
		rotx -= dt;
	if (keys[KEY_EQUALS])
		distance += dt;
	if (keys[KEY_MINUS])
		distance -= dt;
	rotx = fmod(rotx, 2*M_PI);
	roty = fmod(roty, 2*M_PI);
	AM4 mvm = trans(rot(rot(ident(), 1, 0, 0, rotx), 0, 1, 0, roty), 0, 0, distance);
	buffer_AM4(mvm_buf, sizeof(mvm_buf)/sizeof(mvm_buf[0]), mvm);
}
