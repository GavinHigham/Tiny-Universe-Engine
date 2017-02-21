#include "controller.h"

// enum {
// 	LEFTX,
// 	LEFTY,
// 	RIGHTX,
// 	RIGHTY,
// 	TRIGGERLEFT,
// 	TRIGGERRIGHT,
//	NUM_HANDLED_AXES 
// };

Sint16 axes[NUM_HANDLED_AXES] = {0};

void caxisevent(SDL_Event e)
{
	switch (e.caxis.axis) {
	case SDL_CONTROLLER_AXIS_LEFTX:
		axes[LEFTX] = e.caxis.value; break;
	case SDL_CONTROLLER_AXIS_LEFTY:
		axes[LEFTY] = e.caxis.value; break;
	case SDL_CONTROLLER_AXIS_RIGHTX:
		axes[RIGHTX] = e.caxis.value; break;
	case SDL_CONTROLLER_AXIS_RIGHTY:
		axes[RIGHTY] = e.caxis.value; break;
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
		axes[TRIGGERLEFT] = e.caxis.value; break;
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
		axes[TRIGGERRIGHT] = e.caxis.value; break;
	}
}

void jaxisevent(SDL_Event e)
{
	switch (e.jaxis.axis) {
	case 0:
		axes[LEFTX] = e.jaxis.value; break;
	case 1:
		axes[LEFTY] = e.jaxis.value; break;
	case 2:
		axes[RIGHTX] = e.jaxis.value; break;
	case 3:
		axes[RIGHTY] = e.jaxis.value; break;
	case 4:
		axes[TRIGGERLEFT] = e.jaxis.value; break;
	case 5:
		axes[TRIGGERRIGHT] = e.jaxis.value; break;
	}
}

void jbuttonevent(SDL_Event e)
{
	if (e.jbutton.type == SDL_JOYBUTTONDOWN) {
		switch (e.jbutton.button) {
		case 8: axes[TRIGGERLEFT] = 32768; break;
		case 9: axes[TRIGGERRIGHT] = 32768; break;
		}
	} else if (e.jbutton.type == SDL_JOYBUTTONUP) {
		switch (e.jbutton.button) {
		case 8: axes[TRIGGERLEFT] = 0; break;
		case 9: axes[TRIGGERRIGHT] = 0; break;
		}
	}
}