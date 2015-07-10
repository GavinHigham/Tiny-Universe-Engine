#include <SDL2/SDL.h>
#include "keyboard.h"

extern int quit;

void keypressed(SDL_Scancode physical_key)
{
	switch (physical_key) {
	case SDL_SCANCODE_ESCAPE: quit = !quit;
		break;
	default: break;
	}
}