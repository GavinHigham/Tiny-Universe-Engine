#ifndef INIT_H
#define INIT_H
#include <SDL2/SDL.h>

int init(SDL_GLContext *context, SDL_Window **window);
void deinit(SDL_GLContext context, SDL_Window *window);

#endif