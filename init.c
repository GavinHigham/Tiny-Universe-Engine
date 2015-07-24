#include <stdio.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
//#include <SDL2/SDL_opengl.h>
#include <SDL2_image/SDL_image.h>
#include "global_images.h"
#include "init.h"
#include "default_settings.h"
#include "shaders.h"

#define FALSE 0
#define TRUE 1

extern SDL_Window *window;
static SDL_GLContext context = NULL;

GLuint shader_program = 0;
GLuint gVBO = 0;
GLuint gIBO = 0;
GLuint gVAO = 0;
GLint gVertexPos2DLocation = -1;

int init_gl();
int init_glew();
int init_shader();
void printLog(GLuint handle, int is_program);
int init_shader_data(GLuint program, GLuint *vbo, GLuint *ibo, GLint *attribute);

int init()
{
	int error = 0;
	if ((error = SDL_Init(SDL_INIT_EVERYTHING)) != 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return error;
	}
	window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}
	int img_flags = IMG_INIT_PNG;
	if (!(IMG_Init(img_flags) & img_flags)) {
		printf("SD_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
		return -1;
	}

	if (init_gl() != 0) {
		return -1;
	}

	if (init_glew() != 0) {
		return -1;
	}

	if (init_shader(&shader_program, vshader_source, fshader_source) != 0) {
		printf("Unable to initialize shaders!\n");
		return -1;
	}
	if (init_shader_data(shader_program, &gVBO, &gIBO, &gVertexPos2DLocation) != 0) {
		printf("Unable to initialize shader data!\n");
		return -1;
	}
	glGenVertexArrays(1, &gVAO);
	glBindVertexArray(gVAO);

	return error;
}

int init_gl()
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	context = SDL_GL_CreateContext(window);
	if (context == NULL) {
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		return -1;
	}
	if (SDL_GL_SetSwapInterval(1) < 0) {
		printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
	}
	return 0;
}

int init_glew()
{
	glewExperimental = TRUE;
	GLenum glewError = glewInit();
	checkErrors("After glewInit");
	if (glewError != GLEW_OK) {
		printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
		return -1;
	}
	return 0;
}

void deinit()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyWindow(window);
	SDL_GL_DeleteContext(context);
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
	return 0;
}

int init_shader_data(GLuint program, GLuint *vbo, GLuint *ibo, GLint *attribute)
{
	*attribute = glGetAttribLocation(program, "LVertexPos2D");
	if (*attribute == -1) {
		printf("LVertexPos2D is not a valid glsl program variable!\n");
		return -1;
	}
	glClearColor(0.0f, 0.0f, 0.25f, 1.0f);
	glGenBuffers(1, vbo);
	glGenBuffers(1, ibo);
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

void checkErrors(char *label)
{
	int error = glGetError();
	if (error != GL_NO_ERROR) {
		printf("%s: %d\n", label, error);
	}
}