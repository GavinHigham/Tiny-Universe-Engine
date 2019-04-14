#ifndef INIT_H
#define INIT_H
#include <SDL2/SDL.h>

int engine_init();
void engine_deinit();

int gl_init(SDL_GLContext *context, SDL_Window *window);

#endif