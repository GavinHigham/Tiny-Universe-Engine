#ifndef INIT_H
#define INIT_H
#include <SDL2/SDL.h>

int engine_init(SDL_GLContext *context, SDL_Window *window);
void engine_deinit();

#endif