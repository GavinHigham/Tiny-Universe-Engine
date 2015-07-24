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
#define FRAME_TIME_MS 1000.0/FPS

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
static int quit = GL_FALSE;
int SDLCALL quit_event(void *userdata, SDL_Event *e);
Uint32 renderloop_timer(Uint32 interval, void *param);

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
	SDL_AddTimer(FRAME_TIME_MS, renderloop_timer, NULL);
	while (!quit && SDL_WaitEvent(&e)) { //Loop until there's an error, or a desire to quit.
		switch (e.type) {
		case SDL_KEYDOWN: keypressed(e.key.keysym.scancode);
			break;
		case SDL_USEREVENT:
			update(1.0/FPS); //Later on, I will send the actual elapsed time per frame.
			render();
			SDL_GL_SwapWindow(window);
			break;
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

Uint32 renderloop_timer(Uint32 interval, void *param)
{
	//Taken from example code at http://wiki.libsdl.org/SDL_AddTimer
    SDL_Event event;
    SDL_UserEvent userevent;
	
    userevent.type = SDL_USEREVENT;
    userevent.code = 0;
    userevent.data1 = NULL;
    userevent.data2 = NULL;
	
    event.type = SDL_USEREVENT;
    event.user = userevent;
	
    SDL_PushEvent(&event);
    return(interval);
}
