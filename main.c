#include <stdio.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
//#include <SDL2/SDL_opengl.h>
#include <SDL2_image/SDL_image.h>
//Project headers.
#include "main.h"
#include "init.h"
#include "image_load.h"
#include "global_images.h"
#include "keyboard.h"

#define FALSE 0
#define TRUE 1

SDL_Window *window = NULL;
SDL_GLContext context = NULL;
static int quit = GL_FALSE;
int SDLCALL quit_event(void *userdata, SDL_Event *e);

int main()
{
	SDL_Event e;
	atexit(deinit);
	if (init() < 0) {
		printf("Something went wrong in init! Aborting.\n");
		return -1;
	}
	SDL_AddEventWatch(quit_event, NULL);
	while (!quit && SDL_WaitEvent(&e)) { //Loop until there's an error, or a desire to quit.
		switch (e.type) {
		case SDL_KEYDOWN: keypressed(e.key.keysym.scancode);
			break;
		}
		render();
		SDL_GL_SwapWindow(window);
	}

	return 0;
}

void quit_application() {
	quit = TRUE;
}

int SDLCALL quit_event(void *userdata, SDL_Event *e)
{
    if (e->type == SDL_QUIT) {
    	quit_application();
    }
    return 0;
}