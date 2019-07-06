#include "components/components.h"
#define ECS_ACTIVE_ECS E
#define ECS_ACTIVE_CIDS cids
#include "components/components_generic.h"

void component_customdrawable_draw_starbox(ecs_ctx *E, struct entity_cids *cids, uint32_t camera, uint32_t self, void *ctx)
{
	PhysicalTemp *P = entity_physicaltemp(camera);
	Camera *C = entity_camera(camera);

	star_box_update(P->origin); //This should be moved to some kind of updatable/scriptable component.
	star_box_draw(P->origin, C->proj_mat);
}