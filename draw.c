#include "effects.h"
#include "draw.h"
#include "entity/drawable.h"
#include "drawf.h"
#include "math/utility.h"

extern GLfloat proj_view_mat[16];
extern vec3 sun_direction;

void draw(Drawable *drawable)
{
	drawable->draw(drawable->effect, *drawable->bg, *drawable->frame);
}

void prep_matrices(amat4 model_matrix, GLfloat pvm[16], GLfloat mm[16], GLfloat mvpm[16], GLfloat mvnm[16])
{
	amat4_to_array(model_matrix, mm);
	amat4_buf_mult(pvm, mm, mvpm);
	if (mvnm)
		mat3_vec3_to_array(mat3_transp(model_matrix.a), (vec3){0, 0, 0}, mvnm);
}

void draw_forward(EFFECT *e, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat mm[16], mvpm[16], mvnm[16];
	prep_matrices(model_matrix, proj_view_mat, mm, mvpm, mvnm);
	drawf("-m-m-m", e->model_matrix, mm, e->model_view_projection_matrix, mvpm, e->model_view_normal_matrix, mvnm);
	glDrawElements(bg.primitive_type, bg.index_count, GL_UNSIGNED_INT, NULL);
}

void draw_forward_adjacent(EFFECT *e, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat mm[16], mvpm[16], mvnm[16];
	prep_matrices(model_matrix, proj_view_mat, mm, mvpm, mvnm);
	drawf("-m-m-m", e->model_matrix, mm, e->model_view_projection_matrix, mvpm, e->model_view_normal_matrix, mvnm);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.aibo);
	glDrawElements(GL_TRIANGLES_ADJACENCY, bg.index_count*2, GL_UNSIGNED_INT, NULL);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);
	checkErrors("After drawing with aibo");
}

void draw_wireframe(EFFECT *e, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat mm[16], mvpm[16];
	prep_matrices(model_matrix, proj_view_mat, mm, mvpm, NULL);
	drawf("-m", e->model_view_projection_matrix, mvpm);
	glDrawElements(bg.primitive_type, bg.index_count, GL_UNSIGNED_INT, NULL);
}


void draw_skybox_forward(EFFECT *e, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat mm[16], mvpm[16];
	prep_matrices(model_matrix, proj_view_mat, mm, mvpm, NULL);
	drawf("-m-m-*3f-*3f", e->model_matrix, mm, e->model_view_projection_matrix, mvpm, e->camera_position, (float *)&model_matrix.t, e->sun_direction, (float *)&sun_direction);
	glDrawElements(GL_TRIANGLES, bg.index_count, GL_UNSIGNED_INT, NULL);
}