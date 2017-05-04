#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <GL/glew.h>
#include "triangular_terrain_tile.h"
#include "renderer.h"
#include "macros.h"
#include "math/utility.h"
#include "math/geometry.h"
#include "procedural_planet.h"

//Suppress prints
// #define printf(...) while(0) {}
// #define vec3_print(...) while(0) {}
// #define vec3_println(...) while(0) {}

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
	static int tile_index = 0;
	//Could be replaced with a custom allocator in the future.
	tri_tile *new = malloc(sizeof(tri_tile));
	new->is_init = false;
	new->depth = 0; //TODO: Figure out why I put this here, and if I need it here.
	new->tile_index = tile_index++;

	return new;
}

//Creates storage for the positions, normals, and colors, as well as OpenGL handles.
//Should be freed by the caller, using free_tri_tile.
tri_tile * init_tri_tile(tri_tile *t, vec3 vertices[3], bpos_origin sector, int num_rows, void (finishing_touches)(tri_tile *, void *), void *finishing_touches_context)
{
	assert(!t->is_init);
	if (t->is_init) return t;

	//Get counts.
	t->bg.index_count = num_tri_tile_indices(num_rows);
	t->num_vertices   = num_tri_tile_vertices(num_rows);
	t->num_rows = num_rows;
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

	t->override_col = (vec3){1.0, 1.0, 1.0};

	//The new tile origin will be the centroid of the three tile vertices.
	t->centroid = (vertices[0] + vertices[1] + vertices[2]) / 3.0;
	t->normal = vec3_normalize(t->centroid); //This is a sphere normal, TODO: Handle non-sphere case.
	t->sector = sector;
	bpos_split_fix(&t->centroid, &t->sector);

	//Recalculate vertex positions relative to new sector.
	for (int i = 0; i < 3; i++)
		t->tile_vertices[i] = bpos_remap(vertices[i], sector, t->sector);

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
		vec3 discard;

		//Find procedural heights, and add them.
		pos1      = basis_y * height(pos1, &discard) + pos1;
		pos2      = basis_y * height(pos2, &discard) + pos2;
		*position = basis_y * height(*position, &discard) + *position;
		//Compute the normal.
		*normal = vec3_normalize(vec3_cross(pos1 - *position, pos2 - *position));
		return position->y;
}

//Given a particular tri tile row, and point coordinates (x, y),
//Return the vertex indices of the triangle containing the specified point.
//(0, 0) represents the bottom-left, and (1,1) represents top-right of the triangle strip.
void tri_tile_strip_face_at(int row, float x, float y, int *i0, int *i1, int *i2)
{
	int i = (int)(x + y) + (int)(x) + num_tri_tile_indices(row);
	i = (i + 2) < (num_tri_tile_indices(row+1) - 1) ? i : i - 1; //Hacky way to make the indices inclusive with the end of the strip.
	printf("x: %f, y: %f, row: %i, row start: %i\n", x, y, row, num_tri_tile_indices(row));
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
		int strip_row = num_tile_rows * par_t;
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
	vec3 intersection = {0, 0, 0};
	vec3 tile_intersection = {0, 0, 0};

	int result = tri_tile_raycast(t->tile_vertices, t->num_rows, start, dir, &tile_intersection, indices);
	printf("Intersection indices: %i, %i, %i\n", indices[0], indices[1], indices[2]);
	printf("Num indices on this tile: %i\n", t->bg.index_count);

	assert(result == 1);
	assert(indices[0] >= 0);
	assert(indices[2] < t->bg.index_count);

	printf("Vertices: ");
	for (int i = 0; i < 3; i++) {
		positions[i] = t->positions[indices[i]];
		vec3_print(positions[i]);
	}
	printf("\n");

	result = ray_tri_intersect(start, tile_intersection, positions, &intersection);

	if (result == 1) {
		return vec3_dist(start, intersection);
	} else {
		printf("ANOTHER FINE MESS WE'VE GOTTEN INTO\n");
		printf("THIS WILL HELP YOU IN YOUR JOURNEY:\n");
		printf("start: "); vec3_println(start);
		printf("dir:   "); vec3_println(dir);
		printf("tile intersection: "); vec3_println(tile_intersection);
		printf("tile index: %i\n",t->tile_index);
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