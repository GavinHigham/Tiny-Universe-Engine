#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <GL/glew.h>
#include "glla.h"
#include "triangular_terrain_tile.h"
#include "open-simplex-noise-in-c/open-simplex-noise.h"
#include "math/utility.h"
#include "buffer_group.h"
#include "macros.h"
#include "renderer.h"
#include "math/space_sector.h"
#include "procedural_planet.h"

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
//GLuint *shared_tri_tile_indices = NULL;
//int shared_tri_tile_indices_num_rows = 0;
//GLuint shared_tri_tile_indices_buffer_object = 0;
//int shared_tri_tile_indices_buffer_object_rows_buffered = 0;

static GLuint *get_shared_tri_tile_indices(int num_rows)
{
	GLuint *old = shared_tri_tile_ibo.indices;
	if (num_rows > shared_tri_tile_ibo.num_rows)
		shared_tri_tile_ibo.indices = realloc(old, sizeof(GLuint)*num_tri_tile_indices(num_rows));
	if (shared_tri_tile_ibo.indices) {
		tri_tile_indices(shared_tri_tile_ibo.indices, num_rows, shared_tri_tile_ibo.num_rows);
		shared_tri_tile_ibo.num_rows = num_rows;
	} else {
		free(old);
		shared_tri_tile_ibo.num_rows = 0;
	}

	return shared_tri_tile_ibo.indices;
}

static GLuint get_shared_tri_tile_indices_buffer_object(int num_rows)
{
	if (!shared_tri_tile_ibo.buffer_object)
		glGenBuffers(1, &shared_tri_tile_ibo.buffer_object);

	if (num_rows > shared_tri_tile_ibo.rows_buffered) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shared_tri_tile_ibo.buffer_object);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*num_tri_tile_indices(num_rows), get_shared_tri_tile_indices(num_rows), GL_STATIC_DRAW);
	}

	return shared_tri_tile_ibo.buffer_object;
}

//Creates a terrain struct.
tri_tile * new_tri_tile()
{
	//Could be replaced with a custom allocator in the future.
	tri_tile *new = malloc(sizeof(tri_tile));
	new->is_init = false;
	new->depth = 0;

	return new;
}

//Creates storage for the positions, normals, and colors, as well as OpenGL handles.
//Should be freed by the caller, using free_tri_tile.
tri_tile * init_tri_tile(tri_tile *t, vec3 vertices[3], space_sector sector, int num_rows, void (finishing_touches)(tri_tile *, void *), void *finishing_touches_context)
{
	assert(!t->is_init);
	if (t->is_init) return t;

	//Get counts.
	t->bg.index_count = num_tri_tile_indices(num_rows);
	t->num_vertices   = num_tri_tile_vertices(num_rows);
	//Generate storage.
	t->positions = (vec3 *)malloc(sizeof(vec3) * t->num_vertices);
	t->normals   = (vec3 *)malloc(sizeof(vec3) * t->num_vertices);
	t->colors    = (vec3 *)malloc(sizeof(vec3) * t->num_vertices);
	if (!t->positions || !t->normals || !t->colors)
		printf("Malloc didn't work lol\n");

	//Set up GPU buffer storage. I hate how this works and need to simplify it.
	t->bg.primitive_type = GL_TRIANGLE_STRIP;
	glGenVertexArrays(1, &t->bg.vao);
	glBindVertexArray(t->bg.vao);
	glGenBuffers(LENGTH(t->bg.buffer_handles), t->bg.buffer_handles);
	setup_attrib_for_draw(effects.forward.vPos,    t->bg.vbo, GL_FLOAT, 3);
	setup_attrib_for_draw(effects.forward.vNormal, t->bg.nbo, GL_FLOAT, 3);
	setup_attrib_for_draw(effects.forward.vColor,  t->bg.cbo, GL_FLOAT, 3);
	t->buffered = false;

	//Get an appropriately expanded index buffer.
	t->bg.ibo = get_shared_tri_tile_indices_buffer_object(num_rows);

	//The new tile origin will be the centroid of the three tile vertices.
	t->pos = (vertices[0] + vertices[1] + vertices[2]) / 3.0;
	t->sector = sector;
	space_sector_canonicalize(&t->pos, &t->sector);

	//Recalculate vertex positions relative to new sector and origin.
	for (int i = 0; i < 3; i++)
		t->tile_vertices[i] = space_sector_position_relative_to_sector(vertices[i], sector, t->sector) - t->pos;

	//Generate the initial vertex positions, coplanar points on the triangle formed by vertices[3].
	int numverts = tri_tile_vertices(t->positions, num_rows, t->tile_vertices[0], t->tile_vertices[1], t->tile_vertices[2]);

	assert(t->num_vertices == numverts);

	//Run any finishing touches (such as curving the tile onto a planet).
	t->finishing_touches = finishing_touches;
	t->finishing_touches_context = finishing_touches_context;
	t->finishing_touches(t, finishing_touches_context);

	t->is_init = true;

	return t;
}

//Frees the dynamic storage and OpenGL objects held by a terrain struct.
void deinit_tri_tile(tri_tile *t)
{
	if (t->is_init) {
		free(t->positions);
		free(t->normals);
		free(t->colors);
		//free(t->indices);
		//glDeleteBuffers(1, &t->bg.ibo);
		glDeleteVertexArrays(1, &t->bg.vao);
		glDeleteBuffers(LENGTH(t->bg.buffer_handles), t->bg.buffer_handles);
		t->is_init = false;
		t->buffered = false;
	}
}

//Frees a dynamically-allocated tri_tile, deinit-ing it first.
void free_tri_tile(tri_tile *t)
{
	if (t) {
		deinit_tri_tile(t);
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

int tri_tile_vertices(vec3 vertices[], int num_rows, vec3 a, vec3 b, vec3 c)
{
	int written = 0;
	vertices[written++] = a;
	for (int i = 1; i <= num_rows; i++) {
		float f1 = (float)i/(num_rows);
		vec3 left = vec3_lerp(a, b, f1);
		vec3 right = vec3_lerp(a, c, f1);
		for (int j = 0; j <= i; j++) {
			float f2 = (float)j/(i);
			vertices[written++] = vec3_lerp(left, right, f2); 
		}
	}
	return written;
}

//Using height, take position and distort it along the basis vectors, and compute its normal.
//height: A heightmap function which will affect the final position of the vertex along the basis_y vector.
//basis x, basis_y, basis_z: Basis vectors for the vertex.
//position: In/Out, the starting and ending position of the vertex.
//normal: Output for the normal of the vertex.
//Returns the "height", or displacement along basis_y.
float tri_tile_vertex_position_and_normal(height_map_func height, vec3 basis_x, vec3 basis_y, vec3 basis_z, float epsilon, vec3 *position, vec3 *normal)
{
		//Create two points, scootched out along the basis vectors.
		vec3 pos1 = basis_x * epsilon + *position;
		vec3 pos2 = basis_z * epsilon + *position;
		//Find procedural heights, and add them.
		pos1      = basis_y * height(pos1) + pos1;
		pos2      = basis_y * height(pos2) + pos2;
		*position = basis_y * height(*position) + *position;
		//Compute the normal.
		*normal = vec3_normalize(vec3_cross(pos1 - *position, pos2 - *position));
		return position->y;
}

void buffer_tri_tile(tri_tile *t)
{
	GLfloat *positions = malloc(t->num_vertices * 3 * sizeof(GLfloat));
	GLfloat *normals = malloc(t->num_vertices * 3 * sizeof(GLfloat));
	GLfloat *colors = malloc(t->num_vertices * 3 * sizeof(GLfloat));

	for (int i = 0; i < t->num_vertices; i++) {
		vec3_unpack(&positions[i*3], t->positions[i]);
		vec3_unpack(&normals[i*3], t->normals[i]);
		vec3_unpack(&colors[i*3], t->colors[i]);
	}

	glBindVertexArray(t->bg.vao);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIMITIVE_RESTART_INDEX);
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*t->num_vertices, positions, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.nbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*t->num_vertices, normals, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.cbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*t->num_vertices, colors, GL_STATIC_DRAW);
	//Bind buffer to current bound vao so it's used as the index buffer for draw calls.
	glBindBuffer(GL_ARRAY_BUFFER, t->bg.ibo);
	t->buffered = true;
	free(positions);
	free(normals);
	free(colors);
}