#include "entity/physical.h"

void physical_update(Physical *p)
{
	p->velocity.t += p->acceleration.t;
	p->position.t += p->velocity.t;
	p->velocity.a = mat3_mult(p->velocity.a, p->acceleration.a);
	p->position.a = mat3_mult(p->position.a, p->velocity.a);

	bpos_split_fix(&p->position.t, &p->origin);
}