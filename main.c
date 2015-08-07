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
#include "render.h"

#define FALSE 0
#define TRUE 1

#define FPS 60.0
#define MS_PER_SECOND 1000.0
#define FRAME_TIME_MS MS_PER_SECOND/FPS

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
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
	//renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	SDL_AddEventWatch(quit_event, NULL);
	Uint32 last_swap_timestamp = SDL_GetTicks();
	int wake_early_ms = 2;
	while (!quit) { //Loop until quit
		while (SDL_PollEvent(&e)) { //Exhaust our event queue before updating and rendering
			switch (e.type) {
			case SDL_KEYDOWN: keypressed(e.key.keysym.scancode);
				break;
			case SDL_KEYUP: keyreleased(e.key.keysym.scancode);
				break;
			}
		}
		Uint32 ms_since_update = (SDL_GetTicks() - last_swap_timestamp);
		if (ms_since_update >= FRAME_TIME_MS) {
				//Since user input is handled above, game state is "locked" when we enter this block.
				last_swap_timestamp = SDL_GetTicks();
				SDL_GL_SwapWindow(window); //Display a new screen to the user every 16 ms, on the dot.
				update(ms_since_update/MS_PER_SECOND); //At 16 ms intervals, begin an update. HOPEFULLY DOESN'T TAKE MORE THAN 16 MS.
				render(); //This will be a picture of the state as of (hopefully exactly) 16 ms ago.
		} else if ((FRAME_TIME_MS - ms_since_update) > wake_early_ms) { //If there's more than wake_early_ms milliseconds left...
			SDL_Delay(FRAME_TIME_MS - ms_since_update - wake_early_ms); //Sleep up until wake_early_ms milliseconds left. (Busywait the rest)
		}
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
