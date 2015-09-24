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
			GLint projection_matrix;
			GLint gScreenSize;
			GLint uLight_attr;
			GLint uLight_col;
			GLint uLight_pos;
			GLint NMVM;
			GLint MVM;
		};
		GLint unif[10];
	};
};
extern struct shader_prog point_light_program;
extern struct shader_prog deferred_program;
extern struct shader_prog point_light_wireframe_program;
extern struct shader_prog *shader_programs[3];
extern struct shader_info *shader_infos[3];

#endif
