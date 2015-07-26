#include <SDL2/SDL.h>
#include "keyboard.h"
#include "main.h"

#define TRUE 1
#define FALSE 0

extern SDL_Window *window;
int keys[] = {0, 0};

void keypressed(SDL_Scancode physical_key)
{
	switch (physical_key) {
	case SDL_SCANCODE_ESCAPE:
		quit_application();
		break;
	case SDL_SCANCODE_LEFT:
		keys[KEY_LEFT] = TRUE;
		break;
	case SDL_SCANCODE_RIGHT:
		keys[KEY_RIGHT] = TRUE;
		break;
	default: break;
	}
}

void keyreleased(SDL_Scancode physical_key)
{
	static int fullscreen = 0;
	switch (physical_key) {
	case SDL_SCANCODE_F:
		fullscreen ^= SDL_WINDOW_FULLSCREEN_DESKTOP;
		SDL_SetWindowFullscreen(window, fullscreen);
		break;
	case SDL_SCANCODE_LEFT:
		keys[KEY_LEFT] = FALSE;
		break;
	case SDL_SCANCODE_RIGHT:
		keys[KEY_RIGHT] = FALSE;
		break;
	default: break;
	}
}