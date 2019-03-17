#ifndef PHYSICAL_H
#define PHYSICAL_H
#include "glla.h"
#include "math/bpos.h"

typedef struct physical_component {
	float bounding_sphere; //Stored as radius squared, centered on position.t
	amat4 position;
	amat4 velocity;
	amat4 acceleration;
	bpos_origin origin; //May later want to move this into a separate component for performance.
} Physical;

#endif