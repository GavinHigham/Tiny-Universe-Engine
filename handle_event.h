#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include <SDL2/SDL.h>

extern const Uint8 *key_state;
void init_keyboard();
void keyevent(SDL_Keysym keysym, SDL_EventType type);

#endif