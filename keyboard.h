#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <SDL2/SDL.h>

extern int keys[];
enum keys {KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_EQUALS, KEY_MINUS};
void keyevent(SDL_Keysym keysym, SDL_EventType type);

#endif