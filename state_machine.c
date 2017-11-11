#include <assert.h>
#include <inttypes.h>
#include "state_machine.h"

/*
For now, I'll use a state transition table to represent a state machine.
It uses a little more space, and won't be as manageable for large state machines,
but it's easy to author, and easy to reason about storage space.
*/

struct state_machine {
	uint8_t num_states;
	uint8_t num_inputs;
	uint8_t transition_table[];
};

uint16_t state_machine_next(struct state_machine sm, uint8_t state, uint8_t input)
{
	assert(input < sm.num_inputs);
	assert(state < sm.num_states);
	return SM_TRANSITION(state, sm.transition_table[input * sm.num_states + state]);
}