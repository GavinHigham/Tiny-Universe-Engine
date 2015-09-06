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
void axisevent(SDL_Event e)
{
	switch (e.caxis.axis) {
	case SDL_CONTROLLER_AXIS_LEFTX:
		axes[LEFTX] = e.caxis.value;
	break;
	case SDL_CONTROLLER_AXIS_LEFTY:
		axes[LEFTY] = e.caxis.value;
	break;
	case SDL_CONTROLLER_AXIS_RIGHTX:
		axes[RIGHTX] = e.caxis.value;
	break;
	case SDL_CONTROLLER_AXIS_RIGHTY:
		axes[RIGHTY] = e.caxis.value;
	break;
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
		axes[TRIGGERLEFT] = e.caxis.value;
	break;
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
		axes[TRIGGERRIGHT] = e.caxis.value;
	break;
	}
}