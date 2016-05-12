//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#ifndef SHADERS_H
#define SHADERS_H
#include <GL/glew.h>

struct shader_info {
	const GLchar **vs_source;
	const GLchar **fs_source;
	const GLchar **gs_source;
	const GLchar **attr_names;
	const GLchar **unif_names;
	const char **vs_file_path;
	const char **fs_file_path;
	const char **gs_file_path;
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
			GLint zpass;
			GLint projection_view_matrix;
			GLint model_matrix;
			GLint projection_matrix;
			GLint sun_color;
			GLint uLight_attr;
			GLint uLight_col;
			GLint uLight_pos;
			GLint sun_direction;
			GLint model_view_matrix;
			GLint eye_pos;
			GLint gLightPos;
			GLint model_view_normal_matrix;
			GLint ambient_pass;
			GLint uOrigin;
			GLint camera_position;
		};
		GLint unif[16];
	};
};
extern struct shader_prog skybox_program;
extern struct shader_prog outline_program;
extern struct shader_prog forward_program;
extern struct shader_prog shadow_program;
extern struct shader_prog stars_program;
extern struct shader_prog *shader_programs[5];
extern struct shader_info *shader_infos[5];

#endif
