#include <SDL2/SDL.h>
#include "keyboard.h"
#include "main.h"

#define NUM_HANDLED_KEYS 4
#define TRUE 1
#define FALSE 0

extern SDL_Window *window;
int keys[NUM_HANDLED_KEYS] = {FALSE};

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
	case SDL_SCANCODE_UP:
		keys[KEY_UP] = TRUE;
		break;
	case SDL_SCANCODE_DOWN:
		keys[KEY_DOWN] = TRUE;
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
	case SDL_SCANCODE_UP:
		keys[KEY_UP] = FALSE;
		break;
	case SDL_SCANCODE_DOWN:
		keys[KEY_DOWN] = FALSE;
		break;
	default: break;
	}
}