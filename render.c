#include <math.h>
#include <GL/glew.h>
#include "render.h"
#include "init.h"
#include "shaders.h"
#include "shader_program.h"

extern GLuint gVBO, gIBO, gCBO;

void render()
{
	glUseProgram(simple_program.handle);
	checkErrors("After glUseProgram");
	glClear(GL_COLOR_BUFFER_BIT);

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
	static float elapsed_ms;
	elapsed_ms = fmod(elapsed_ms + dt, 2*M_PI);
	GLfloat vertices[] = {
		cos(elapsed_ms)/2,          sin(elapsed_ms)/2,
		cos(elapsed_ms+(M_PI/2))/2, sin(elapsed_ms+(M_PI/2))/2,
		cos(elapsed_ms+M_PI)/2,     sin(elapsed_ms+M_PI)/2,
		cos(elapsed_ms-(M_PI/2))/2, sin(elapsed_ms-(M_PI/2))/2
	};
	GLfloat colors[] = {
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
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