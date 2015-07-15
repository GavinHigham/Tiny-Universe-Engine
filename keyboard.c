#include <SDL2/SDL.h>
#include "keyboard.h"
#include "main.h"

extern SDL_Window *window;

void keypressed(SDL_Scancode physical_key)
{
	static int fullscreen = 0;
	switch (physical_key) {
	case SDL_SCANCODE_ESCAPE:
		quit_application();
		break;
	case SDL_SCANCODE_F:
		fullscreen ^= SDL_WINDOW_FULLSCREEN_DESKTOP;
		SDL_SetWindowFullscreen(window, fullscreen);
		break; 
	default: break;
	}
}