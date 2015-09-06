#include <stdio.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
//#include <SDL2/SDL_opengl.h>
#include <SDL2_image/SDL_image.h>
#include "global_images.h"
#include "init.h"
#include "default_settings.h"
#include "shaders.h"
#include "render.h"

#define FALSE 0
#define TRUE 1

extern SDL_Window *window;
static SDL_GLContext context = NULL;

GLuint gVBO = 0;
GLuint gIBO = 0;
GLuint gCBO = 0;
GLuint gVAO = 0;

int init_gl();
int init_glew();
int init_shader_program(struct shader_prog *program, struct shader_info info);
int init_shader_attributes(struct shader_prog *program, struct shader_info info);
int init_shader_uniforms(struct shader_prog *program, struct shader_info info);
int init_shaders(struct shader_prog **programs, struct shader_info **infos, int numprogs);
void printLog(GLuint handle, int is_program);

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

	if (init_gl()) {
		return -1;
	}

	if (init_glew()) {
		return -1;
	}

	if (init_shaders(shader_programs, shader_infos, sizeof(shader_programs)/sizeof(shader_programs[0]))) {
		printf("Something went wrong with shader initialization!\n");
		return -1;
	}

	SDL_GameControllerEventState(SDL_ENABLE);
	/* Open the first available controller. */
	SDL_GameController *controller = NULL;
	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
	    if (SDL_IsGameController(i)) {
	        controller = SDL_GameControllerOpen(i);
	        if (controller) {
	            break;
	        } else {
	            fprintf(stderr, "Could not open gamecontroller %i: %s\n", i, SDL_GetError());
	        }
	    }
	}
	
	glGenVertexArrays(1, &gVAO);
	glBindVertexArray(gVAO);

	init_render(); //Located in render.c

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

int init_shader_program(struct shader_prog *program, struct shader_info info)
{
	program->handle = glCreateProgram();
	//Vertex shader part
	GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader, 1, info.vs_source, NULL);
	glCompileShader(vshader);

	GLint success = FALSE;
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
	if (success != TRUE) {
		printf("Unable to compile vertex shader %d!\n", vshader);
		printLog(vshader, FALSE);
		return -1;
	}
	glAttachShader(program->handle, vshader);
	//Fragment shader part
	const GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader, 1, info.fs_source, NULL);
	glCompileShader(fshader);

	success = FALSE;
	glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
	if (success != TRUE) {
		printf("Unable to compile fragment shader %d!\n", fshader);
		printLog(fshader, FALSE);
		return -1;
	}
	glAttachShader(program->handle, fshader);
	glLinkProgram(program->handle);
	success = TRUE;
	glGetProgramiv(program->handle, GL_LINK_STATUS, &success);
	if (success != TRUE) {
		printf("Unable to link program %d!\n", program->handle);
		printLog(program->handle, TRUE);
		return -1;
	}
	glDeleteShader(vshader);
	glDeleteShader(fshader);
	return 0;
}

int init_shader_attributes(struct shader_prog *program, struct shader_info info)
{
	for (int i = 0; i < sizeof(program->attr)/sizeof(program->attr[0]); i++) {
		if (program->attr[i] != -1) {
			program->attr[i] = glGetAttribLocation(program->handle, info.attr_names[i]);
			if (program->attr[i] == -1) {
				printf("%s is not a valid glsl program variable! It may have been optimized out.\n", info.attr_names[i]);
				return -1;
			}
		}
	}
	return 0;
}

int init_shader_uniforms(struct shader_prog *program, struct shader_info info)
{
	for (int i = 0; i < sizeof(program->unif)/sizeof(program->unif[0]); i++) {
		if (program->unif[i] != -1) {
			program->unif[i] = glGetUniformLocation(program->handle, info.unif_names[i]);
			if (program->unif[i] == -1) {
				printf("%s is not a valid glsl uniform variable! It may have been optimized out.\n", info.unif_names[i]);
				return -1;
			}
		}
	}
	return 0;
}

int init_shader(struct shader_prog *program, struct shader_info info)
{
	if (init_shader_program(program, info)) {
		printf("Could not compile shader program.\n");
		return -1;
	}
	if (init_shader_attributes(program, info)) {
		printf("Could not retrieve shader attributes.\n");
		return -1;
	}
	if (init_shader_uniforms(program, info)) {
		printf("Could not retrieve shader uniforms.\n");
		return -1;
	}
	return 0;
}

int init_shaders(struct shader_prog **programs, struct shader_info **infos, int numprogs)
{
	for (int i = 0; i < numprogs; i++) {
		int result = init_shader(programs[i], *infos[i]);
		if (result)
			return result;
	}
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