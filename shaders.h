//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#ifndef SHADERS_H
#define SHADERS_H
#include <GL/glew.h>

struct shader_info {
	const GLchar **vs_source;
	const GLchar **fs_source;
	const GLchar **attr_names;
	const GLchar **unif_names;
	const char **vs_file_path;
	const char **fs_file_path;
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
			GLint gPositionMap;
			GLint gNormalMap;
			GLint gColorMap;
			GLint model_matrix;
			GLint projection_matrix;
			GLint gScreenSize;
			GLint diffuse_light;
			GLint specular_light;
			GLint uLight_attr;
			GLint uLight_col;
			GLint uLight_pos;
			GLint model_view_matrix;
			GLint MVM;
			GLint model_view_normal_matrix;
			GLint transp_model_matrix;
			GLint camera_position;
		};
		GLint unif[16];
	};
};
extern struct shader_prog effects_program;
extern struct shader_prog point_light_program;
extern struct shader_prog deferred_program;
extern struct shader_prog point_light_wireframe_program;
extern struct shader_prog forward_program;
extern struct shader_prog *shader_programs[5];
extern struct shader_info *shader_infos[5];

#endif
