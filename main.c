#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
//#include <SDL2/SDL_opengl.h>
#include <SDL2_image/SDL_image.h>
//Project headers.
#include "init.h"
#include "image_load.h"
#include "keyboard.h"
#include "render.h"
#include "controller.h"
#include "default_settings.h"

enum {
	MS_PER_SECOND = 1000
};
static int fps = 60; //Frames per second.
static int wake_early_ms = 2; //How many ms early should the main loop wake from sleep to avoid oversleeping.

//Average number of tight loop iterations. Global so it can be accessed from keyboard.c
int loop_iter_ave = 0;
SDL_Renderer *renderer = NULL;
SDL_Window *window = NULL;

//Callback for close button event.
int SDLCALL quit_event(void *userdata, SDL_Event *e)
{
    if (e->type == SDL_QUIT)
    	*((sig_atomic_t *)userdata) = true;

    return 0;
}

int main()
{
	SDL_Event e;
	SDL_GLContext context;

	sig_atomic_t quit = false; //Use an atomic so that changes from another thread will be seen.

	int result = 0;
	if ((result = SDL_Init(SDL_INIT_EVERYTHING)) != 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return result;
	}

	window = SDL_CreateWindow(
		"Creative Title",
		WINDOW_OFFSET_X,
		WINDOW_OFFSET_Y,
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		WINDOW_FLAGS);
	if (window == NULL) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return -1;
	}

	result = init_engine(&context, window);
	if (result < 0) {
		printf("Something went wrong in init_engine! Aborting.\n");
		return -1;
	}

	//renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	SDL_AddEventWatch(quit_event, &quit);

	Uint32 windowID = SDL_GetWindowID(window);
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
			case SDL_WINDOWEVENT:
				if (e.window.windowID == windowID) {
            		switch (e.window.event)  {
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						handle_resize(e.window.data1, e.window.data2);
		                break;
		            }
	            }
			}
		}

		int frame_time_ms = MS_PER_SECOND/fps;
		Uint32 since_update_ms = (SDL_GetTicks() - last_swap_timestamp);

		if (since_update_ms >= frame_time_ms - 1) {
				//Since user input is handled above, game state is "locked" when we enter this block.
				last_swap_timestamp = SDL_GetTicks();
		 		SDL_GL_SwapWindow(window); //Display a new screen to the user every 16 ms, on the dot.
				update((float)since_update_ms/MS_PER_SECOND); //At 16 ms intervals, begin an update. HOPEFULLY DOESN'T TAKE MORE THAN 16 MS.
				render(); //This will be a picture of the state as of (hopefully exactly) 16 ms ago.
				//Get a rolling average of the number of tight loop iterations per frame.
				loop_iter_ave = (loop_iter_ave + loop_iter)/2; //Average the current number of loop iterations with the average.
				loop_iter = 0;
		} else if ((frame_time_ms - since_update_ms) > wake_early_ms) { //If there's more than wake_early_ms milliseconds left...
			SDL_Delay(frame_time_ms - since_update_ms - wake_early_ms); //Sleep up until wake_early_ms milliseconds left. (Busywait the rest)
		}
	}
	
	deinit_render();
	SDL_DestroyWindow(window);
	SDL_GL_DeleteContext(context);
	deinit_engine();
	return 0;
}
