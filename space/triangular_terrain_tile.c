#include "triangular_terrain_tile.h"
#include "space_scene.h"
#include "macros.h"
#include "mesh.h"
#include "math/utility.h"
#include "math/geometry.h"
#include "procedural_planet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <GL/glew.h>

//Suppress prints
#define printf(...) while(0) {}
#define vec3_print(...) while(0) {}
#define vec3_println(...) while(0) {}

//If I add margins to all my heightmaps later then I can avoid A LOT of work.
extern int PRIMITIVE_RESTART_INDEX;
extern struct osn_context *osnctx;

//The index buffer of a triangular tile of n rows contains that of one for n+1 rows.
//Memoize the largest requested index buffer and store here for reuse.
struct {
	GLuint *indices;
	GLuint buffer_object;
	int num_rows;
	int rows_buffered;
} shared_tri_tile_ibo = {NULL, 0, 0, 0};

//TODO(Gavin): Make thi static again.
GLuint **get_shared_tri_tile_indices(int num_rows)
{
	GLuint *old = shared_tri_tile_ibo.indices;
	if (num_rows > shared_tri_tile_ibo.num_rows) {
		shared_tri_tile_ibo.indices = realloc(old, sizeof(GLuint)*num_tri_tile_indices(num_rows));
		if (shared_tri_tile_ibo.indices) {
			tri_tile_indices(shared_tri_tile_ibo.indices, num_rows, shared_tri_tile_ibo.num_rows);
			shared_tri_tile_ibo.num_rows = num_rows;
		} else {
			free(old);
			shared_tri_tile_ibo.num_rows = 0;
		}
	}

	return &shared_tri_tile_ibo.indices;
}

GLuint get_shared_tri_tile_indices_buffer_object(int num_rows)
{
	if (!shared_tri_tile_ibo.buffer_object)
		glGenBuffers(1, &shared_tri_tile_ibo.buffer_object);

	if (num_rows > shared_tri_tile_ibo.rows_buffered) {
		GLuint *indices = *get_shared_tri_tile_indices(num_rows);
		if (shared_tri_tile_ibo.num_rows == num_rows) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shared_tri_tile_ibo.buffer_object);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*num_tri_tile_indices(num_rows), indices, GL_STATIC_DRAW);
			shared_tri_tile_ibo.rows_buffered = num_rows;
		}
	}

	return shared_tri_tile_ibo.buffer_object;
}

//Creates a terrain struct.
tri_tile * tri_tile_new(const struct tri_tile_big_vertex big_vertices[3])
//tri_tile * tri_tile_new(vec3 vertices[3])
{
	static int tile_index = 0;
	//Could be replaced with a custom allocator in the future.
	tri_tile *t = malloc(sizeof(tri_tile));
	assert(t);
	t->is_init = false;
	t->tile_index = tile_index++;
	memcpy(t->big_vertices, big_vertices, 3*sizeof(struct tri_tile_big_vertex));
	t->centroid = (t->big_vertices[0].position + t->big_vertices[1].position + t->big_vertices[2].position) / 3.0;
	t->radius = fmax(fmax(
		vec3_dist(t->centroid, t->big_vertices[0].position),
		vec3_dist(t->centroid, t->big_vertices[1].position)),
		vec3_dist(t->centroid, t->big_vertices[2].position)
	);

	return t;
}

//TODO(Gavin): Split this into tile data, mesh generation, and OpenGL setup functions.
//Creates storage for the positions, normals, and colors, as well as OpenGL handles.
//Should be freed by the caller, using tri_tile_free.
tri_tile * tri_tile_init(tri_tile *t, qvec3 offset, int num_rows, void (finishing_touches)(tri_tile *, void *), void *finishing_touches_context)
{
	assert(!t->is_init);
	if (t->is_init) return t;

	//Get counts.
	t->num_indices = num_tri_tile_indices(num_rows);
	t->num_vertices = num_tri_tile_vertices(num_rows);
	t->num_rows = num_rows;
	t->mesh = (struct tri_tile_vertex *)malloc(sizeof(struct tri_tile_vertex) * t->num_vertices);
	if (!t->mesh)
		printf("Malloc didn't work lol\n");

	glGenVertexArrays(1, &t->vao);
	glBindVertexArray(t->vao);

	glGenBuffers(1, &t->mesh_buffer);
	glEnableVertexAttribArray(effects.forward.vPos);
	glEnableVertexAttribArray(effects.forward.vNormal);
	glEnableVertexAttribArray(effects.forward.vColor);
	glBindBuffer(GL_ARRAY_BUFFER, t->mesh_buffer);
	glVertexAttribPointer(effects.forward.vPos, 3, GL_FLOAT, GL_FALSE,
		sizeof(struct tri_tile_vertex), (void *)offsetof(struct tri_tile_vertex, position));
	glVertexAttribPointer(effects.forward.vNormal, 3, GL_FLOAT, GL_FALSE,
		sizeof(struct tri_tile_vertex), (void *)offsetof(struct tri_tile_vertex, normal));
	glVertexAttribPointer(effects.forward.vColor, 3, GL_FLOAT, GL_FALSE,
		sizeof(struct tri_tile_vertex), (void *)offsetof(struct tri_tile_vertex, color));

	t->buffered = false;

	//Get an appropriately expanded index buffer.
	t->ibo = get_shared_tri_tile_indices_buffer_object(num_rows);

	t->override_col = (vec3){1.0, 1.0, 1.0};
	t->offset = offset;
	bpos_split_fix(&t->centroid, &t->offset);

	//Recalculate vertex positions relative to new offset.
	for (int i = 0; i < 3; i++)
		t->big_vertices[i].position = bpos_remap((bpos){t->big_vertices[i].position, offset}, t->offset);

	//Generate the initial vertex positions and texture coordinates, spaced along the triangle formed by big_vertices[3].
	int numverts = tri_tile_mesh_init(t->mesh, num_rows, t->big_vertices);

	assert(t->num_vertices == numverts);

	//Run any finishing touches (such as curving the tile onto a planet).
	t->finishing_touches = finishing_touches;
	t->finishing_touches_context = finishing_touches_context;
	t->finishing_touches(t, finishing_touches_context);

	t->is_init = true;

	return t;
}

//Frees the dynamic storage and OpenGL objects held by a terrain struct.
void tri_tile_deinit(tri_tile *t)
{
	if (t->is_init) {
		free(t->mesh);
		//free(t->indices);
		//glDeleteBuffers(1, &t->bg.ibo);
		glDeleteVertexArrays(1, &t->vao);
		glDeleteBuffers(1, &t->mesh_buffer);
		//glDeleteBuffers(LENGTH(t->bg.buffer_handles), t->bg.buffer_handles);
		t->is_init = false;
		t->buffered = false;
	}
}

//Frees a dynamically-allocated tri_tile, deinit-ing it first.
void tri_tile_free(tri_tile *t)
{
	if (t) {
		tri_tile_deinit(t);
		free(t);
	}
}

float tri_height_map(vec3 pos)
{
	float scale = 0.01;
	float amplitude = TERRAIN_AMPLITUDE;
	float height = 0;
	for (int i = 1; i < 2; i++) {
		height += (open_simplex_noise3(osnctx, pos.x*scale, pos.y*scale, pos.z*scale) + 1)/pow(2, i);
		scale *= 2;
	}
	return amplitude * pow(height, 4) - amplitude / 2;
}

float tri_height_map_flat(vec3 pos)
{
	return 1;
}

int num_tri_tile_indices(int num_rows)
{
	return num_rows*num_rows + 3*num_rows;
}

int num_tri_tile_vertices(int num_rows)
{
	return (num_rows+2)*(num_rows+1)/2;
}

int tri_tile_indices(GLuint indices[], int num_rows, int start_row)
{
	int written = num_tri_tile_indices(start_row); //Figure out the actual correct math for this.
	for (int i = start_row; i < num_rows; i++) {
		int start_index = ((i+2)*(i+1))/2;
		for (int j = start_index; j < start_index + i + 1; j++) {
			indices[written++] = j;
			indices[written++] = j - i - 1;
		}
		indices[written++] = start_index + i + 1;
		indices[written++] = PRIMITIVE_RESTART_INDEX;
	}
	return written - num_tri_tile_indices(start_row);
}

int tri_tile_mesh_init(struct tri_tile_vertex mesh[], int num_rows, struct tri_tile_big_vertex big_vertices[3])
{
	#define LERP(a, b, alpha) (((1 - alpha) * a) + (alpha * b))
	int written = 0;
	mesh[written].position = big_vertices[0].position;
	memcpy(mesh[written].tx, big_vertices[0].tx, 2*sizeof(float));
	written++;
	//Row by row, from top to bottom.
	for (int i = 1; i <= num_rows; i++) {
		float f1 = (float)i/(num_rows);
		vec3 left = vec3_lerp(big_vertices[0].position, big_vertices[1].position, f1);
		vec3 right = vec3_lerp(big_vertices[0].position, big_vertices[2].position, f1);
		//Along each row, from left to right.
		for (int j = 0; j <= i; j++) {
			float f2 = (float)j/i;
			float f3 = (float)j/num_rows;
			mesh[written].position = vec3_lerp(left, right, f2);
			mesh[written].tx[0] = LERP(big_vertices[0].tx[1], big_vertices[2].tx[1], f3);
			mesh[written].tx[1] = LERP(big_vertices[0].tx[1], big_vertices[2].tx[1], 1-f1);
			written++;
		}
	}
	return written;
}

struct tri_tile_big_vertex tri_tile_get_big_vert_average(tri_tile *t, int v1, int v2)
{
	return (struct tri_tile_big_vertex){
		(t->big_vertices[v1].position + t->big_vertices[v2].position)/2,
		{
			(t->big_vertices[v1].tx[0] + t->big_vertices[v2].tx[0])/2,
			(t->big_vertices[v1].tx[1] + t->big_vertices[v2].tx[1])/2
		}
	};
}

//Given a particular tri tile row, and point coordinates (x, y),
//Return the vertex indices of the triangle containing the specified point.
//(0, 0) represents the bottom-left, and (1,1) represents top-right of the triangle strip.
void tri_tile_strip_face_at(int row, float x, float y, int *i0, int *i1, int *i2)
{
	int i = floorf(x + y) + floorf(x) + num_tri_tile_indices(row);
	int next_row_start = num_tri_tile_indices(row+1);
	i = (i + 3) < next_row_start ? i : next_row_start - 4; //Hacky way to make the indices inclusive with the end of the strip.
	//printf("x: %f, y: %f, row: %i, row start: %i\n", x, y, row, num_tri_tile_indices(row));
	*i0 = i;
	*i1 = i + 1;
	*i2 = i + 2;
}

//Find the vertex indices of the triangle intersecting a ray cast into t.
int tri_tile_raycast(vec3 tri_vertices[3], int num_tile_rows, vec3 start, vec3 dir, vec3 *intersection, int indices[3])
{
	float par_s, par_t; //Parametric coordinates of the point of intersection on the tile.
	int result = ray_tri_intersect_with_parametric(start, dir, tri_vertices, intersection, &par_s, &par_t);
	if (result == 1) {
		par_t = 1 - par_t; //TODO: Handle literal edge cases.
		float x = par_s * num_tile_rows; //x goes from 0 to row_base along the bottom of the triangle strip.
		float y = ceil(num_tile_rows * par_t) - (num_tile_rows * par_t); //y goes from 0 to 1 from the bottom to the top of the triangle strip.
		//It's possible to be exactly at the base of the bottom strip, in which case our index math betrays us. Death to the traitor.
		int strip_row = floorf(num_tile_rows * par_t);
		strip_row = strip_row < num_tile_rows ? strip_row : num_tile_rows - 1;
		tri_tile_strip_face_at(strip_row, x, y, &indices[0], &indices[1], &indices[2]);
	}

	return result;
}

//Returns the depth of a ray cast into the tile t, or infinity if there is no intersection.
float tri_tile_raycast_depth(tri_tile *t, vec3 start, vec3 dir)
{
	int indices[3];
	vec3 positions[3];
	vec3 vertices[3] = {t->big_vertices[0].position, t->big_vertices[1].position, t->big_vertices[2].position};
	vec3 intersection = {0, 0, 0};
	vec3 tile_intersection = {0, 0, 0};

	int result = tri_tile_raycast(vertices, t->num_rows, start, dir, &tile_intersection, indices);

	assert(result == 1);
	assert(indices[0] >= 0);
	assert(indices[2] < t->num_indices);

	GLuint **global_indices = get_shared_tri_tile_indices(t->num_rows);
	
	//printf("Vertices: ");
	for (int i = 0; i < 3; i++) {
		int pos_i = *global_indices[indices[i]];
		assert(pos_i != PRIMITIVE_RESTART_INDEX);
		positions[i] = t->mesh[pos_i].position;
		//vec3_print(positions[i]);
	}
	//printf("\n");

	result = ray_tri_intersect(start, dir, positions, &intersection);

	if (result == 1) {
		return vec3_dist(start, intersection);
	} else {
		//We could be inside the planet, in which case point the ray backwards.
		result = ray_tri_intersect(start, -dir, positions, &intersection);
		if (result == 1)
			return -vec3_dist(start, intersection);
		//If not, print debug info.
		printf("ANOTHER FINE MESS WE'VE GOTTEN INTO\n");
		printf("THIS WILL HELP YOU IN YOUR JOURNEY:\n");
		printf("start: "); vec3_println(start);
		printf("dir:   "); vec3_println(dir);
		printf("tile intersection: "); vec3_println(tile_intersection);
		printf("tile index: %i\n",t->tile_index);
		printf("Intersection indices: %i, %i, %i\n", indices[0], indices[1], indices[2]);
		printf("Num indices on this tile: %i\n", t->bg.index_count);
		printf("Vertex buffer indices: %u, %u, %u\n", global_indices[indices[0]], global_indices[indices[1]], global_indices[indices[2]]);
	}
	
	return INFINITY;
}

void tri_tile_raycast_test()
{
	int indices[3] = {0, 0, 0};
	vec3 vertices[] = {{0, 0, -10}, {-5, 0, 0}, {5, 0, 0}};
	vec3 ray_start = (vec3){0,  10, -2};
	vec3 ray_end   = (vec3){0, -10, -2};
	vec3 intersection = {0, 0, 0};
	int result = ray_tri_intersect(ray_start, ray_end, vertices, &intersection);
	assert(result == 1);

	result = tri_tile_raycast(vertices, 2, ray_start, ray_end, &intersection, indices);
	printf("tri_tile_raycast_test->%i, expect (5, 6, 7): (%i, %i, %i)\n", result, indices[0], indices[1], indices[2]);
	assert(result == 1);
}

void tri_tile_buffer(tri_tile *t)
{
	glBindVertexArray(t->vao);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);
	//Bind buffer to current bound vao so it's used as the index buffer for draw calls.
	glBindBuffer(GL_ARRAY_BUFFER, t->mesh_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(struct tri_tile_vertex)*t->num_vertices, t->mesh, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, t->ibo);
	t->buffered = true;
}