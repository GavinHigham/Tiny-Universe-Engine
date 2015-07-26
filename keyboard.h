#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <SDL2/SDL.h>

extern int keys[];
enum keys {KEY_LEFT, KEY_RIGHT};
void keypressed(SDL_Scancode physical_key);
void keyreleased(SDL_Scancode physical_key);

#endif