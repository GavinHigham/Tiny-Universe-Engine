#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <SDL2/SDL.h>

enum keys {KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_EQUALS, KEY_MINUS, NUM_HANDLED_KEYS};
extern int keys[NUM_HANDLED_KEYS];
void keyevent(SDL_Keysym keysym, SDL_EventType type);

#endif