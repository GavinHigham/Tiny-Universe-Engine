#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
//#include <SDL2/SDL_opengl.h>
#include <SDL2_image/SDL_image.h>
#include "init.h"
#include "default_settings.h"
#include "shaders/shaders.h"
#include "shader_utils.h"
#include "gl_utils.h"
#include "render.h"
#include "macros.h"
#include "func_list.h"
#include "keyboard.h"

int init_gl(SDL_GLContext *context, SDL_Window *window);
int init_glew();

void reload_effects_void_wrapper()
{
	if (!reload_effects(shader_programs, shader_infos, LENGTH(shader_programs))) {
		deinit_render();
		init_render();
	}
}

static void reload_signal_handler(int signo) {
	printf("Received SIGUSR1! Reloading shaders!\n");
	func_list_add(&update_func_list, 1, reload_effects_void_wrapper);
}

int init(SDL_GLContext *context, SDL_Window **window)
{
	int error = 0;
	if ((error = SDL_Init(SDL_INIT_EVERYTHING)) != 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return error;
	}
	*window = SDL_CreateWindow("Sock Engine", WINDOW_OFFSET_X, WINDOW_OFFSET_Y, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}
	int img_flags = IMG_INIT_PNG;
	if (!(IMG_Init(img_flags) & img_flags)) {
		printf("SD_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
		return -1;
	}

	if (init_gl(context, *window)) {
		return -1;
	}

	if (init_glew()) {
		return -1;
	}

	if (init_effects(shader_programs, shader_infos, LENGTH(shader_programs))) {
		printf("Something went wrong with shader program initialization!\n");
		return -1;
	}

	SDL_GameControllerEventState(SDL_ENABLE);
	/* Open the first available controller. */
	SDL_GameController *controller = NULL;
	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
		printf("Testing controller %i\n", i);
		if (SDL_IsGameController(i)) {
			controller = SDL_GameControllerOpen(i);
			if (controller) {
				printf("Successfully opened controller %i\n", i);
				break;
			} else {
				printf("Could not open gamecontroller %i: %s\n", i, SDL_GetError());
			}
		} else {
			printf("Controller %i is not a controller?\n", i);
		}
	}

	init_keyboard();
	init_render(); //Located in render.c

	if (signal(SIGUSR1, reload_signal_handler) == SIG_ERR) {
		printf("An error occurred while setting a signal handler.\n");
	}

	return error;
}

int init_gl(SDL_GLContext *context, SDL_Window *window)
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);

	*context = SDL_GL_CreateContext(window);
	if (*context == NULL) {
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
	glewExperimental = true;
	GLenum glewError = glewInit();
	checkErrors("After glewInit");
	if (glewError != GLEW_OK) {
		printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
		return -1;
	}
	return 0;
}

void deinit(SDL_GLContext context, SDL_Window *window)
{
	SDL_DestroyWindow(window);
	SDL_GL_DeleteContext(context);
	IMG_Quit();
	SDL_Quit();
}