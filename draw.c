#include "effects.h"
#include "draw.h"
#include "entity/drawable.h"
#include "drawf.h"
#include "macros.h"
#include "math/utility.h"
#include "experiments/spiral_scene.h"
#include "space/galaxy_volume.h"

extern GLfloat proj_mat[16];
extern GLfloat skybox_proj_mat[16];
extern GLfloat proj_view_mat[16];
extern vec3 sun_direction;

// void draw(Drawable *drawable)
// {
// 	drawable->draw(drawable->effect, *drawable->bg, *drawable->frame);
// }

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

#include "input_event.h"
bool skybox_point_in_viewport(float *mvpm, float *vpos, float *out)
{
	float vpos_buf[4] = {vpos[0], vpos[1], vpos[2], 1};
	float gl_Position[4];
	bool tmp = true;
	amat4_buf_multpoint(mvpm, vpos_buf, gl_Position);//vec4(p,1);//vec4(normalize(position) * pow(tex.r-tex.g, 0.03), 1);
	vpos_buf[2] = vpos_buf[3];
	// gl_Position[2] = (log2(fmax(/*1e-6*/0.000001, 1.0 + gl_Position[2])) * log_depth_intermediate_factor - 1.0) * gl_Position[3];
	for (int i = 0; i < 3; i++) {
		gl_Position[i] /= gl_Position[3];
		if (gl_Position[i] > 1.0 || gl_Position[i] < -1.0) {
			tmp = false;
			// break;
		}
	}

	if (out)
		memcpy(out, gl_Position, 3 * sizeof(float));

	return tmp;
}

extern amat4 eye_frame;
extern amat4 inv_eye_frame;
extern float screen_width, screen_height;

void draw_skybox_forward(EFFECT *e, struct buffer_group bg, amat4 model_matrix)
{
	// amat4_print(model_matrix);
	// puts("");
	// model_matrix = (amat4){MAT3_IDENT};
	glBindVertexArray(bg.vao);
	GLfloat mm[16], mvpm[16], pm[16];

	//Compute inverse eye frame.
	{
		float tmp[16];
		amat4_to_array((amat4){inv_eye_frame.a}, tmp);
		amat4_buf_mult(skybox_proj_mat, tmp, pm);
	}
	// memcpy(pm, proj_view_mat, sizeof(pm));
	// pm[3] = pm[7] = pm[11] = 0.0; //Discard the translation portion
	prep_matrices(model_matrix, pm, mm, mvpm, NULL);
	// puts("pm:");
	// amat4_buf_print(pm);
	// puts("");
	// puts("mm:");
	// amat4_print(model_matrix);
	// puts("");
	// amat4_buf_print(mm);
	// puts("proj_view_mat:");
	// amat4_buf_print(proj_view_mat);
	// puts("mvpm:");
	// amat4_buf_print(mvpm);
	// mvpm[3] = mvpm[7] = mvpm[11] = 0.0;

	if (key_state[SDL_SCANCODE_4]) {

		float clip_coords[3];
		float vpos[3] = {1.0, 1.0, -1.0};

		bool in_viewport = skybox_point_in_viewport(mvpm, vpos, clip_coords);
		uint32_t hash = float3_hash(clip_coords, 18) % 230 + 1;
		//vec3 colors[] = {color_from_float3(clip_coords, 10), color_from_float3(gpu_clip_coords, 10)};

		printf(ANSI_NUMERIC_FCOLOR "%s: % f, % f, % f" ANSI_COLOR_RESET "\n",
			hash,
			in_viewport     ? "" : "~",     clip_coords[0],     clip_coords[1],     clip_coords[2]);
	}

	drawf("-m-m-*3f-*3f", e->model_matrix, mm, e->model_view_projection_matrix, mvpm, e->camera_position, (float *)&model_matrix.t, e->sun_direction, (float *)&sun_direction);
	if (key_state[SDL_SCANCODE_9]) {
		glDepthFunc(GL_GREATER);
	} else {
		glDepthFunc(GL_LEQUAL);
	}

	static vec3 last_cubemap_loc = (vec3){0.0, 0.0, 0.0};
	static bool rendered_once = false;
	static int num_frames = 1;
	// if (!rendered_once || vec3_dist(last_cubemap_loc, eye_frame.t) > 100.0) {
		num_frames = galaxy_no_worries_just_render_cubemap(!rendered_once);
		last_cubemap_loc = eye_frame.t;
		glViewport(0, 0, screen_width, screen_height);
		glUseProgram(e->handle);
		rendered_once = true;
	// }
	galaxy_bind_cubemap();
	glBindVertexArray(bg.vao);
	glUniform1i(e->accum_cube, 0);
	glUniform1i(e->num_frames_accum, num_frames);
	checkErrors("After cubemap setup");

	glDrawElements(bg.primitive_type, bg.index_count, GL_UNSIGNED_INT, NULL);
	checkErrors("After draw skybox");
}