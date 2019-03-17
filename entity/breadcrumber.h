#ifndef BREADCRUMBER_H
#define BREADCRUMBER_H
#include "entity/entity.h"
#include "math/bpos.h"
#include <stdbool.h>

typedef struct breadcrumber_component {
	bpos *crumbs; //This breadcrumber's circular buffer of crumbs.
	int start; //Index of the current head of this breadcrumber's circular buffer of crumbs, relative to buffer.
	int num; //Number of crumbs in this breadcrumber's circular buffer.
	int max; //Max number of crumbs in this breadcrumber's circular buffer
	double threshold; //If the entity is more than this distance away from the previous crumb, leave a new crumb.
	bool crumb_left_last_update;
} Breadcrumber;

//Leaves a crumb, if the entity is more than a certain distance from the previous breadcrumb.
void breadcrumber_try_crumb(Entity *entity);

//Probably need a more complete API here

#endif