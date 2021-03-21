#include "systems/ply_mesh_renderer.h"

/*
This system will know how to render an entity with a PlyMesh component and a Physical component into a framebuffer.

A mesh without normals or vertex colors can have them generated.
Some additional configuration information will be used to determine if the mesh is to be rendered
with textures, transparent, if it should receive shadows, etc. Where this information will be stored
has not been decided yet.


UGH UGH UGH HOW DO I START

Okay, here's how I want the code to work:

uint32_t spaceship = entity_new();
entity_add(spaceship, (PlyMesh){.name = "newship.ply"});
entity_add(spaceship, (Drawable){...});
entity_add(spaceship, (Physical){...});

When the PlyMesh component is added, it does a lookup on the name to see if we've already loaded up
that model already. If we have, then it just assignes the ply_mesh * pointer. If not, it tries to
load it. It will also check to see if the model has all the required properties per-vertex. If not,
it will try to generate them. If it can't, it can either crash (debug mode, when I add one) or
remove the PlyMesh component from the entity (release mode, again, when I add one).

The PLY Mesh Renderer system will find all entities with the required components, sort them by some
"sort key", load their vertex data into a big VBO, load their indices into a big IBO, and then
prepare everything needed to draw them in a "pass" later (I may need to draw them all multiple times
 to do depth pre-pass, etc.)

Eventually culling will go in here too :)
*/

struct ply_mesh_renderer_ctx {

};

struct ply_mesh * ply_mesh_renderer_get_cached_mesh(struct ply_mesh_renderer_ctx *ctx, const char *filename)
{
	
}

struct ply_mesh * ply_mesh_renderer_get_mesh(struct ply_mesh_renderer_ctx *ctx, const char *filename)
{
	struct ply_mesh *mesh = ply_mesh_renderer_get_cached_mesh(ctx, filename);
	if (mesh)
		return mesh;

	mesh = ply_mesh_load(filename, PLY_LOAD_GEN_IB & PLY_LOAD_GEN_AIB & PLY_LOAD_GEN_NORMALS);


}