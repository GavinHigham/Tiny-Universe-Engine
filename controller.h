#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <SDL2/SDL.h>

enum {
	LEFTX,
	LEFTY,
	RIGHTX,
	RIGHTY,
	TRIGGERLEFT,
	TRIGGERRIGHT,
	NUM_HANDLED_AXES
};
extern Sint16 axes[NUM_HANDLED_AXES];

void axisevent(SDL_Event e);

#endif