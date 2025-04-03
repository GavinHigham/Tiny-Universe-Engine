#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
//#include <SDL3/SDL_opengl.h>
#include "graphics.h"
#include "init.h"
#include "math/utility.h"
#include "macros.h"
#include "input_event.h"
#include "open-simplex-noise-in-c/open-simplex-noise.h"

int open_simplex_noise_seed = 83619; //No special significance, I just mashed on the keyboard.
struct osn_context *osnctx;

int gl_init(SDL_GLContext *context, SDL_Window *window);
int glad_init();

int engine_init()
{
	open_simplex_noise(open_simplex_noise_seed, &osnctx);

	SDL_SetGamepadEventsEnabled(true);
	input_event_init();

	return 0;
}

int gl_init(SDL_GLContext *context, SDL_Window *window)
{
	#define GOTO_ERR_ON_FAIL(x) if (!x) goto error;
	GOTO_ERR_ON_FAIL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3))
	GOTO_ERR_ON_FAIL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3))
	GOTO_ERR_ON_FAIL(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE))
	GOTO_ERR_ON_FAIL(SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8))
	GOTO_ERR_ON_FAIL(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32))

	*context = SDL_GL_CreateContext(window);
	if (!*context) {
error:
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		return -1;
	}
	if (!SDL_GL_SetSwapInterval(1)) {
		printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
	}

	if (glad_init())
		return -1;
	checkErrors("glad_init");

	return 0;
}

int glad_init()
{
	int version = gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);
	if (version == 0) {
	    printf("Failed to initialize OpenGL context\n");
	    return -1;
	}
	return 0;
}

void engine_deinit()
{
	input_event_deinit();
	open_simplex_noise_free(osnctx);
	SDL_Quit();
}