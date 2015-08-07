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

extern GLuint gVBO, gIBO, gCBO;
static GLfloat mvm_buf[16] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};
GLfloat distance = -2.0;

/*
.75 0  0 0
0   1  0 0
0   0  2 3
0   0 -1 1
*/

void buffer_cube() {
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
		0, 1, 2, 3, 4, 5, 6, 1, //Need to stop and draw a second fan here.
		7, 6, 1, 2, 3, 4, 5, 6
	};
	/*
	GLuint triangles_indices[] = {
		0, 1, 2,
		0, 2, 3,
		0, 3, 4,
		0, 4, 5,
		0, 5, 6,
		0, 6, 1
	};
	*/
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, gCBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
}

void draw_cube()
{
	//vertex_pos
	glEnableVertexAttribArray(simple_program.attr[vertex_pos]);
	checkErrors("After glEnableVertexAttribArray");
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glVertexAttribPointer(simple_program.attr[vertex_pos], 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//vColor
	glEnableVertexAttribArray(simple_program.attr[vColor]);
	glBindBuffer(GL_ARRAY_BUFFER, gCBO);
	glVertexAttribPointer(simple_program.attr[vColor], 3, GL_FLOAT, GL_FALSE, 0, NULL);
	//indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
	glDrawElements(GL_TRIANGLE_FAN, 8, GL_UNSIGNED_INT, NULL);
	glDrawElements(GL_TRIANGLE_FAN, 8, GL_UNSIGNED_INT, (void *)8);
	checkErrors("After glDrawElements");

	glDisableVertexAttribArray(simple_program.attr[vertex_pos]);
	glDisableVertexAttribArray(simple_program.attr[vColor]);
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

void render()
{
	glUseProgram(simple_program.handle);
	checkErrors("After glUseProgram");
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLfloat proj_mat[16];
	make_projection_matrix(FOV, (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT, -1, -10, proj_mat, sizeof(proj_mat)/sizeof(proj_mat[0]));
	checkErrors("After glUniform1f for distance");
	glUniformMatrix4fv(simple_program.unif[projection_matrix], 1, GL_TRUE, proj_mat);
	glUniformMatrix4fv(simple_program.unif[model_view_matrix], 1, GL_TRUE, mvm_buf);

	draw_cube();

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
	if (keys[KEY_UP])
		distance += dt;
	if (keys[KEY_DOWN])
		distance -= dt;
	rot = fmod(rot, 2*M_PI);
	AM4 mvm = trans(rotAM4(0, 1, 0, rot), 0, 0, distance);
	//AM4 mvm = {{1, 0, 0, 0, 1, 0, 0, 0, 1}};
	//printAM4(mvm);
	buffer_AM4(mvm_buf, sizeof(mvm_buf)/sizeof(mvm_buf[0]), mvm);
	buffer_cube();
	/*
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
	*/
}