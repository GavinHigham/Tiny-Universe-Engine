#include <stdio.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2_image/SDL_image.h>
//Project headers.
#include "main.h"
#include "init.h"
#include "image_load.h"
#include "global_images.h"
#include "keyboard.h"

SDL_Window *window = NULL;
SDL_GLContext context = NULL;
int quit = GL_FALSE;

int main()
{
	SDL_Event e;
	atexit(deinit);
	if (init() < 0) {
		printf("Something went wrong in init! Aborting.\n");
		return -1;
	}

	//screen_surface = SDL_GetWindowSurface(window);
	while (!quit && SDL_WaitEvent(&e) != 0) { //Loop until there's a quit event, or an error.
		switch (e.type) {
		case SDL_QUIT: quit = GL_TRUE;
			break;
		case SDL_KEYDOWN: keypressed(e.key.keysym.scancode);
			break;
		}
		render();
	}

	return 0;
}