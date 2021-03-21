#ifndef UNIVERSE_ECS_H
#define UNIVERSE_ECS_H 

#include "datastructures/ecs.h"
extern struct entity_ctypes universe_ecs_ctypes;
extern ecs_ctx *puniverse_ecs_ctx;

#define ECS_ACTIVE_ECS puniverse_ecs_ctx
#define ECS_ACTIVE_CTYPES (&universe_ecs_ctypes)
#include "experiments/universe_scene/components_generic.h"

#endif