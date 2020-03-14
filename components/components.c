#include "components.h"
#include "math/utility.h"

//Since every CustomDrawable is "custom", call the per-instance constructor.
CHANGEME(customdrawable_constructor)
{
	CustomDrawable *cd = ecs_entity_add_copy_component(E, eid, ctype, c);
	if (cd->construct)
		cd->construct(E, eid, ctype, cd);
	return cd; //Should this return the result of the previous call instead? Will probably always be the same.
}

//Since every CustomDrawable is "custom", call the per-instance destructor.
CHANGEME(customdrawable_destructor)
{
	CustomDrawable *cd = c;
	if (cd->destruct)
		cd->destruct(E, eid, ctype, c);
	//FINISH THIS
	return NULL;
}

CD(camera_constructor)
{
	Camera *c = component;
	printf("Camera constructor ran\n");
	make_projection_matrix(c->fov, c->aspect, c->near, c->far, c->proj_mat);
	c->log_depth_intermediate_factor = 2.0/log2(fabs(c->far) + 1.0);
}

CD(label_constructor)
{
	Label *l = component;
	if (l->name)
		l->name = strdup(l->name);
	if (l->description)
		l->description = strdup(l->description);
}

CD(label_destructor)
{
	Label *l = component;
	free(l->name);
	free(l->description);
}