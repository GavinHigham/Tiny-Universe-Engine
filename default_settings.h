#ifndef DEFAULT_SETTINGS_H
#define DEFAULT_SETTINGS_H
#include <SDL2/SDL.h>

//Default settings for the program should be specified here.
//In the future, I may wish to make defaults be set from some configuration file.

//WINDOW_FLAGS is a define to ensure it is appropriately typed as a Uint32
#define WINDOW_FLAGS (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE)
enum default_settings {
	SCREEN_WIDTH  = 800,
	SCREEN_HEIGHT = 600,
	WINDOW_OFFSET_X = SDL_WINDOWPOS_UNDEFINED,
	WINDOW_OFFSET_Y = SDL_WINDOWPOS_UNDEFINED
};

#endif