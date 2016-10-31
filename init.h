#ifndef INIT_H
#define INIT_H
#include <SDL2/SDL.h>

int init_engine(SDL_GLContext *context, SDL_Window *window);
void deinit_engine();

#endif