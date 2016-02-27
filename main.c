#include <stdio.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
//#include <SDL2/SDL_opengl.h>
#include <SDL2_image/SDL_image.h>
//Project headers.
#include "main.h"
#include "init.h"
#include "image_load.h"
#include "keyboard.h"
#include "render.h"
#include "controller.h"

#define FPS 60.0
#define MS_PER_SECOND 1000.0
#define FRAME_TIME_MS MS_PER_SECOND/FPS
#define WAKE_EARLY_MS 2 //How many ms early should the main loop wake from sleep to avoid oversleeping.

//Average number of tight loop iterations. Global so it can be accessed from keyboard.c
int loop_iter_ave = 0;
SDL_Renderer *renderer = NULL;
SDL_Window *window = NULL;
//When quit is set to true, the main loop will break, quitting the program.
bool quit = GL_FALSE;
//quit_event will be called when the user clicks the close button.
int SDLCALL quit_event(void *userdata, SDL_Event *e);

int main()
{
	SDL_Event e;
	SDL_GLContext context;

	if (init(&context, &window) < 0) {
		printf("Something went wrong in init! Aborting.\n");
		return -1;
	}
	//renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	SDL_AddEventWatch(quit_event, NULL);
	Uint32 last_swap_timestamp = SDL_GetTicks();
	int loop_iter = 0;
	while (!quit) { //Loop until quit
		loop_iter++; //Count how many times we loop per frame.
		while (SDL_PollEvent(&e)) { //Exhaust our event queue before updating and rendering
			switch (e.type) {
			case SDL_KEYDOWN: keyevent(e.key.keysym, (SDL_EventType)e.type);
				break;
			case SDL_KEYUP: keyevent(e.key.keysym, (SDL_EventType)e.type);
				break;
			case SDL_CONTROLLERAXISMOTION: axisevent(e);
				break;
			}
		}
		Uint32 ms_since_update = (SDL_GetTicks() - last_swap_timestamp);
		if (ms_since_update >= FRAME_TIME_MS - 1) {
				//Since user input is handled above, game state is "locked" when we enter this block.
				last_swap_timestamp = SDL_GetTicks();
				SDL_GL_SwapWindow(window); //Display a new screen to the user every 16 ms, on the dot.
				update(ms_since_update/MS_PER_SECOND); //At 16 ms intervals, begin an update. HOPEFULLY DOESN'T TAKE MORE THAN 16 MS.
				render(); //This will be a picture of the state as of (hopefully exactly) 16 ms ago.
				//Get a rolling average of the number of tight loop iterations per frame.
				loop_iter_ave = (loop_iter_ave + loop_iter)/2; //Average the current number of loop iterations with the average.
				loop_iter = 0;
		} else if ((FRAME_TIME_MS - ms_since_update) > WAKE_EARLY_MS) { //If there's more than WAKE_EARLY_MS milliseconds left...
			SDL_Delay(FRAME_TIME_MS - ms_since_update - WAKE_EARLY_MS); //Sleep up until WAKE_EARLY_MS milliseconds left. (Busywait the rest)
		}
	}
	deinit(context, window);
	return 0;
}

void quit_application() {
	quit = true;
}
//Callback for close button event.
int SDLCALL quit_event(void *userdata, SDL_Event *e)
{
    if (e->type == SDL_QUIT) {
    	quit_application();
    }
    return 0;
}
