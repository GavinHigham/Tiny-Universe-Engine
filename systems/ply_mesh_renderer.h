#ifndef PLY_MESH_RENDERER_H
#define PLY_MESH_RENDERER_H
#include <inttypes.h>
#include "datastructures/hashtable.h"
#include "datastructures/hmempool.h"

struct ply_mesh_renderer_ctx {
	hashtable *mesh_cache;
	struct hmempool mesh_handles;
};
typedef uint32_t ply_mesh_handle;
struct ply_mesh_renderer_ctx ply_mesh_renderer_new(size_t num_meshes);
void ply_mesh_renderer_delete(struct ply_mesh_renderer_ctx *ctx);
uint32_t ply_mesh_renderer_get_mesh(struct ply_mesh_renderer_ctx *ctx, const char *filename);

#endif