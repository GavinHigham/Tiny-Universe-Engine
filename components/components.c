#include "components.h"
#include "math/utility.h"

//Since every CustomDrawable is "custom", call the per-instance constructor.
CD(customdrawable_constructor)
{
	CustomDrawable *cd = component;
	if (cd->construct)
		cd->construct(eid, component, userdata);
}

//Since every CustomDrawable is "custom", call the per-instance destructor.
CD(customdrawable_destructor)
{
	CustomDrawable *cd = component;
	if (cd->destruct)
		cd->destruct(eid, component, userdata);
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
