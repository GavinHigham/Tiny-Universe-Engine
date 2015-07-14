#include <stdio.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2_image/SDL_image.h>
#include "global_images.h"
#include "init.h"
#include "default_settings.h"

#define FALSE 0
#define TRUE 1

extern SDL_Window *window;
extern SDL_GLContext context;

GLuint shader_program;
GLuint gVBO = 0;
GLuint gIBO = 0;
GLint gVertexPos2DLocation = -1;

int init_shader();
void printLog(GLuint handle, int is_program);
void render();

int init()
{
	int error = 0;
	if ((error = SDL_Init(SDL_INIT_VIDEO)) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return error;
	}
	window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (window == NULL) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}
	int img_flags = IMG_INIT_PNG;
	if (!(IMG_Init(img_flags) & img_flags)) {
		printf("SD_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	context = SDL_GL_CreateContext(window);
	if (context == NULL) {
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		return -1;
	}
	glewExperimental = TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) {
		printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
		return -1;
	}
	if (SDL_GL_SetSwapInterval(1) < 0) {
		printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
	}

	const GLchar *vshader_source[] = {"#version 140\nin vec2 LVertexPos2D; void main() { gl_Position = vec4(LVertexPos2D.x, LVertexPos2D.y, 0, 1); }"};
	const GLchar *fshader_source[] = {"#version 140\nout vec4 LFragment; void main() { LFragment = vec4(1.0, 1.0, 1.0, 1.0); }"};
	if (init_shader(&shader_program, vshader_source, fshader_source) < 0) {
		printf("Unable to initialize shaders!\n");
		return -1;
	}

	return error;
}

void deinit()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
}

int init_shader(GLuint *program, const GLchar *vshader_source[], const GLchar *fshader_source[])
{
	*program = glCreateProgram();
	//Vertex shader part
	GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader, 1, vshader_source, NULL);
	glCompileShader(vshader);

	GLint success = FALSE;
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
	if (success != TRUE) {
		printf("Unable to compile vertex shader %d!\n", vshader);
		printLog(vshader, FALSE);
		return -1;
	}
	glAttachShader(*program, vshader);
	//Fragment shader part
	const GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader, 1, fshader_source, NULL);
	glCompileShader(fshader);
	success = FALSE;
	glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
	if (success != TRUE) {
		printf("Unable to compile fragment shader %d!\n", fshader);
		printLog(fshader, FALSE);
		return -1;
	}
	glAttachShader(*program, fshader);
	glLinkProgram(*program);
	success = TRUE;
	glGetProgramiv(*program, GL_LINK_STATUS, &success);
	if (success != TRUE) {
		printf("Unable to link program %d!\n", *program);
		printLog(*program, TRUE);
		return -1;
	}
	gVertexPos2DLocation = glGetAttribLocation(*program, "LVertexPos2D");
	if (gVertexPos2DLocation == -1) {
		printf("LVertexPos2D is not a valid glsl program variable!\n");
		return -1;
	}
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	GLfloat vertexData[] = 
	{
		-0.5f, -0.5f,
		 0.5f, -0.5f,
		 0.5f,  0.5f,
		-0.5f,  0.5f
	};
	GLuint indexData[] = {0, 1, 2, 3};
	glGenBuffers(1, &gVBO);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);
	glGenBuffers(1, &gIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indexData, GL_STATIC_DRAW);
	return 0;
}

void printLog(GLuint handle, int is_program)
{
	//Why write two log printing functions when you can use FUNCTION POINTERS AND TERNARY OPERATORS >:D
	GLboolean (*glIsLOGTYPE)(GLuint)                                  = is_program? glIsProgram         : glIsShader;
	void (*glGetLOGTYPEiv)(GLuint, GLenum, GLint *)                   = is_program? glGetProgramiv      : glGetShaderiv;
	void (*glGetLOGTYPEInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *) = is_program? glGetProgramInfoLog : glGetShaderInfoLog;

	if (glIsLOGTYPE(handle)) {
		int log_length = 0;
		int max_length = log_length;
		glGetLOGTYPEiv(handle, GL_INFO_LOG_LENGTH, &max_length);
		char *info_log = (char *)malloc(max_length);
		glGetLOGTYPEInfoLog(handle, max_length, &log_length, info_log);
		if (log_length > 0) {
			printf("%s\n", info_log);
		}
		free(info_log);
	} else {
		printf("Name %d is not a %s.\n", handle, is_program?"program":"shader");
	}
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shader_program);
	glEnableVertexAttribArray(gVertexPos2DLocation);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glVertexAttribPointer(gVertexPos2DLocation, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
	glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);
	glDisableVertexAttribArray(gVertexPos2DLocation);
	glUseProgram(0);
}


