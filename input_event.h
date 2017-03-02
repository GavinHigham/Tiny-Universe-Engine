#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include <SDL2/SDL.h>

extern const Uint8 *key_state;

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

void init_keyboard();
void caxisevent(SDL_Event e);
void jaxisevent(SDL_Event e);
void jbuttonevent(SDL_Event e);
void keyevent(SDL_Keysym keysym, SDL_EventType type);

#endif