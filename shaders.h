//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#ifndef SHADERS_H
#define SHADERS_H
#include <GL/glew.h>

struct shader_info {
	const GLchar **vs_source;
	const GLchar **fs_source;
	const GLchar **attr_names;
	const GLchar **unif_names;
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
			GLint projection_matrix;
			GLint emit;
			GLint light_pos;
			GLint alpha;
			GLint NMVM;
			GLint MVM;
		};
		GLint unif[6];
	};
};
extern struct shader_prog lighting_program;
extern struct shader_prog deferred_program;
extern struct shader_prog simple_program;
extern struct shader_prog *shader_programs[3];
extern struct shader_info *shader_infos[3];

#endif
