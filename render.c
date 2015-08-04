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

#define FOV M_PI/2.0

extern GLuint gVBO, gIBO, gCBO;

/*
.75 0  0 0
0   1  0 0
0   0  2 3
0   0 -1 1
*/

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

void render()
{
	glUseProgram(simple_program.handle);
	checkErrors("After glUseProgram");
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLfloat proj_mat[16];
	make_projection_matrix(FOV, (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, 0, -100, proj_mat, sizeof(proj_mat)/sizeof(proj_mat[0]));
	glUniformMatrix4fv(simple_program.unif[projection_matrix], 1, GL_TRUE, proj_mat);

	glEnableVertexAttribArray(simple_program.attr[LVertexPos2D]);
	checkErrors("After glEnableVertexAttribArray");
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glVertexAttribPointer(simple_program.attr[LVertexPos2D], 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(simple_program.attr[vColor]);
	glBindBuffer(GL_ARRAY_BUFFER, gCBO);
	glVertexAttribPointer(simple_program.attr[vColor], 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);
	checkErrors("After glDrawElements");

	glDisableVertexAttribArray(simple_program.attr[LVertexPos2D]);
	checkErrors("After glDisableVertexAttribArray");
	glUseProgram(0);
}

void update(float dt)
{
	static float rot;
	if (keys[KEY_LEFT])
		rot += dt;
	if (keys[KEY_RIGHT])
		rot -= dt;
	rot = fmod(rot, 2*M_PI);
	GLfloat vertices[] = {
		cos(rot)/2,          sin(rot)/2,
		cos(rot+(M_PI/2))/2, sin(rot+(M_PI/2))/2,
		cos(rot+M_PI)/2,     sin(rot+M_PI)/2,
		cos(rot-(M_PI/2))/2, sin(rot-(M_PI/2))/2
	};
	GLfloat colors[] = {
		0.0, 1.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 1.0,
		1.0, 1.0, 1.0
	};
	GLuint indices[] = {0, 1, 2, 3};
	
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, gCBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}