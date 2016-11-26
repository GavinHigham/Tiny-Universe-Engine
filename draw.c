#include "effects.h"
#include "draw.h"
#include "drawable.h"
#include "drawf.h"
#include "gl_utils.h"

extern GLfloat proj_view_mat[16];
extern vec3 sun_direction;

void draw(Drawable *drawable)
{
	drawable->draw(drawable->effect, *drawable->bg, *drawable->frame);
}

void draw_forward(EFFECT *e, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat mm[16];
	GLfloat mvpm[16];
	GLfloat nmvm[16];
	amat4_to_array(model_matrix, mm);
	amat4_buf_mult(proj_view_mat, mm, mvpm);
	mat3_vec3_to_array(mat3_transp(model_matrix.a), (vec3){{0, 0, 0}}, nmvm);
	drawf("-m-m-m", e->model_matrix, mm, e->model_view_projection_matrix, mvpm, e->model_view_normal_matrix, nmvm);
	glDrawElements(bg.primitive_type, bg.index_count, GL_UNSIGNED_INT, NULL);
}

void draw_forward_adjacent(EFFECT *e, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat mvpm[16];
	GLfloat mm[16];
	GLfloat mvnm[16];
	amat4_to_array(model_matrix, mm);
	amat4_buf_mult(proj_view_mat, mm, mvpm);
	mat3_vec3_to_array(mat3_transp(model_matrix.a), (vec3){{0, 0, 0}}, mvnm);
	drawf("-m-m-m", e->model_matrix, mm, e->model_view_projection_matrix, mvpm, e->model_view_normal_matrix, mvnm);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.aibo);
	glDrawElements(GL_TRIANGLES_ADJACENCY, bg.index_count*2, GL_UNSIGNED_INT, NULL);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);
	checkErrors("After drawing with aibo");
}

void draw_wireframe(EFFECT *e, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat mvpm[16];
	GLfloat model_matrix_buf[16];
	amat4_to_array(model_matrix, model_matrix_buf);
	amat4_buf_mult(proj_view_mat, model_matrix_buf, mvpm);
	drawf("-m", e->model_view_projection_matrix, mvpm);
	glDrawElements(bg.primitive_type, bg.index_count, GL_UNSIGNED_INT, NULL);
}


void draw_skybox_forward(EFFECT *e, struct buffer_group bg, amat4 model_matrix)
{
	glBindVertexArray(bg.vao);
	GLfloat mvpm[16];
	GLfloat mm[16];
	amat4_to_array(model_matrix, mm);
	amat4_buf_mult(proj_view_mat, mm, mvpm);
	drawf("-m-m-*3f-*3f", e->model_matrix, mm, e->model_view_projection_matrix, mvpm, e->camera_position, model_matrix.T, e->sun_direction, sun_direction.A);
	glDrawElements(GL_TRIANGLES, bg.index_count, GL_UNSIGNED_INT, NULL);
}