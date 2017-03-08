#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
//#include <SDL2/SDL_opengl.h>
#include <SDL2_image/SDL_image.h>
#include "init.h"
#include "default_settings.h"
#include "effects.h"
#include "shader_utils.h"
#include "gl_utils.h"
#include "renderer.h"
#include "macros.h"
#include "input_event.h"
#include "open-simplex-noise-in-c/open-simplex-noise.h"

int open_simplex_noise_seed = 83619; //No special significance, I just mashed on the keyboard.
struct osn_context *osnctx;

int gl_init(SDL_GLContext *context, SDL_Window *window);
int glew_init();

static void reload_signal_handler(int signo) {
	printf("Received SIGUSR1! Reloading shaders!\n");
	renderer_queue_reload();
}

int engine_init(SDL_GLContext *context, SDL_Window *window)
{
	int img_flags = IMG_INIT_PNG;
	if (!(IMG_Init(img_flags) & img_flags)) {
		printf("SD_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
		return -1;
	}

	if (gl_init(context, window))
		return -1;

	if (glew_init())
		return -1;

	load_effects(
		effects.all,       LENGTH(effects.all),
		shader_file_paths, LENGTH(shader_file_paths),
		attribute_strings, LENGTH(attribute_strings),
		uniform_strings,   LENGTH(uniform_strings));

	open_simplex_noise(open_simplex_noise_seed, &osnctx);

	SDL_GameControllerEventState(SDL_ENABLE);
	input_event_init();

	if (signal(SIGUSR1, reload_signal_handler) == SIG_ERR) {
		printf("An error occurred while setting a signal handler.\n");
	}

	return 0;
}

int gl_init(SDL_GLContext *context, SDL_Window *window)
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

	checkErrors("After init_gl");
	return 0;
}

int glew_init()
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

void engine_deinit()
{
	open_simplex_noise_free(osnctx);
	IMG_Quit();
	SDL_Quit();
}