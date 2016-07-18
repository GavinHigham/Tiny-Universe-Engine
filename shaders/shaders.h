//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#ifndef SHADERS_H
#define SHADERS_H
#include <GL/glew.h>

enum {MAX_SHADERS_PER_EFFECT = 3};
const GLchar **effect_attribute_names;
const GLchar **effect_uniform_names;

struct shader_info {
	const GLchar **shader_texts[MAX_SHADERS_PER_EFFECT];
	GLenum shader_types[MAX_SHADERS_PER_EFFECT];
	char **file_paths[MAX_SHADERS_PER_EFFECT];
};
struct shader_prog {
	GLuint handle;
	union {
		struct {
			GLint vNormal;
			GLint vColor;
			GLint vPos;
		};
		GLint attr[3];
	};
	union {
		struct {
			GLint projection_view_matrix;
			GLint zpass;
			GLint model_matrix;
			GLint sun_color;
			GLint uLight_attr;
			GLint uLight_col;
			GLint uLight_pos;
			GLint model_view_projection_matrix;
			GLint sun_direction;
			GLint eye_pos;
			GLint gLightPos;
			GLint model_view_normal_matrix;
			GLint ambient_pass;
			GLint uOrigin;
			GLint camera_position;
		};
		GLint unif[15];
	};
};
extern struct shader_prog skybox_program;
extern struct shader_prog outline_program;
extern struct shader_prog forward_program;
extern struct shader_prog shadow_program;
extern struct shader_prog stars_program;
extern struct shader_prog wireframe_program;
extern struct shader_prog *shader_programs[6];
extern struct shader_info *shader_infos[6];

#endif
