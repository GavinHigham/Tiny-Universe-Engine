#ifndef INIT_H
#define INIT_H
#include <SDL2/SDL.h>

int init(
	SDL_GLContext *context,
	SDL_Window **window,
	//Window parameters:
	char *title,
	//offset
	int x,
	int y,
	//dimensions
	int w,
	int h,
	//SDL flags
	Uint32 wflags);
void deinit(SDL_GLContext context, SDL_Window *window);

#endif