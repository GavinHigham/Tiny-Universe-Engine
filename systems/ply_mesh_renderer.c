#include "models/ply_mesh.h"
#include "systems/ply_mesh_renderer.h"
#include "components/components.h"
#include "datastructures/hashtable.h"

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

2022-07-18:
Basic rendering can be done by creating a VAO, then essentially translating the PLY elements into
 glVertexAttribPointer calls (where the properties within those elements are essentially x,y,z or r,g,b).

Only supporting indexed rendering for now is simplest.

In order for "render this thing in O(1)", either the entity needs to maintain information from the renderer
(VAO, buffer offset, vertex or index count, etc.) or it needs a handle so that the renderer can maintain that
information itself. Fortunately I already have hmempool, which I can use for this purpose.

2022-07-21
I've got a hashtable for storing the parsed PLY meshes, now I need to figure out what data the mesh_handles
 (name could be changed) point to.
*/

//Idk what these are even for, I guess I just want a simple straightforward way to render a mesh before I build
//up the more complicated mesh-merging / culling / whatever system.
enum PLY_MESH_TYPE {
	PLY_MESH_TYPE_NONE = 0,
	PLY_MESH_TYPE_SIMPLE = 1
};

struct ply_mesh_ctx {
	enum PLY_MESH_TYPE type;
	struct ply_mesh *mesh;
	GLuint vao, aibo, ibo;
};

struct ply_mesh_renderer_ctx ply_mesh_renderer_new(size_t num_meshes)
{
	return (struct ply_mesh_renderer_ctx){
		.mesh_cache = hashtable_new(num_meshes),
		.mesh_handles = hmempool_new(num_meshes, sizeof(struct ply_mesh_ctx))
	};
}

void ply_mesh_renderer_delete(struct ply_mesh_renderer_ctx *ctx)
{
	struct mempool *pool = &ctx->mesh_handles.pool;
	for (int i = 0; i < pool->num; i++)
		ply_mesh_free(((struct ply_mesh_ctx *)pool->pool)[i].mesh);
	hmempool_delete(&ctx->mesh_handles);
	hashtable_free(ctx->mesh_cache, NULL, NULL);
}

uint32_t ply_mesh_renderer_get_mesh(struct ply_mesh_renderer_ctx *ctx, const char *filename)
{
	hashtable_listnode *node = hashtable_find(ctx->mesh_cache, filename, true);
	if (node) {
		if (!node->handle)
			node->handle = hmempool_add(&ctx->mesh_handles,
				&(struct ply_mesh_ctx){
					.mesh = ply_mesh_load(filename, PLY_LOAD_GEN_IB | PLY_LOAD_GEN_AIB | PLY_LOAD_GEN_NORMALS)
				});
		return node->handle;
	}

	//Something went wrong with creating the hashtable listnode
	fprintf(stderr, "Could not find or create PLY mesh cache hashtable listnode.\n");
	return 0;
}

/*
How do I want this to work? Something like:
entity_add(this_ship, (PlyMesh){.filename = "spaceship.ply"});
entity_add(that_ship, (PlyMesh){.filename = "spaceship.ply"});

Then maybe both entities receive the same mesh handle.

Or maybe I can do:
entity_add(some_ship, PlyMesh("spaceship.ply"));

So maybe looking up by filename should return the handle, then the handle can access various data about the mesh?

Probably also want a way to render a bunch of meshes with a bunch of lights from a given camera and framebuffer,
possibly sorting them by depth and culling. How will I handle shaders? If I want to override the shader for a certain
entity, what do I do, and how is it used?

PLY meshes are mainly going to be for ships, trees, foliage, buildings, space stations, debris, missiles, etc.
Maybe I'll want flashing lights or decals. Could possibly get away with just two shaders, one for alpha stuff and
one without. How does that affect how I render?

Maybe another system will be responsible for generating the potentially-visible-set, sorting by depth, doing some
CPU culling, and passing the list to the PLY renderer. In that case, it can just loop through the provided meshes
and draw them.
*/

void ply_mesh_renderer_configure_simple(struct ply_mesh_renderer_ctx *ctx)
{

}

//Render a single mesh into a specific framebuffer, lit by a single light.
// void ply_mesh_renderer_render_simple(struct ply_mesh_renderer_ctx *ctx, hframebuffer fbo, ply_mesh_handle mesh, Camera *c, Emissive *light)
// {

// }
