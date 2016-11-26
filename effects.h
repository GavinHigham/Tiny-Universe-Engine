//AUTO-GENERATED FILE, CHANGES MAY BE OVERWRITTEN.

#ifndef EFFECTS_H
#define EFFECTS_H

#include <GL/glew.h>
typedef struct effect_data {
	GLuint handle;
	union {
		struct {
			GLint ambient_pass;
			GLint camera_position;
			GLint eye_pos;
			GLint gLightPos;
			GLint model_matrix;
			GLint model_view_normal_matrix;
			GLint model_view_projection_matrix;
			GLint projection_view_matrix;
			GLint sun_color;
			GLint sun_direction;
			GLint uLight_attr;
			GLint uLight_col;
			GLint uLight_pos;
			GLint uOrigin;
			GLint zpass;
		};
		GLint unif[15];
	};
	union {
		struct {
			GLint vColor;
			GLint vNormal;
			GLint vPos;
		};
		GLint attr[3];
	};
} EFFECT;

union effect_list {
	struct {
		EFFECT forward;
		EFFECT outline;
		EFFECT shadow;
		EFFECT skybox;
		EFFECT stars;
	};
	EFFECT all[5];
};

union effect_list effects;

const char *uniform_strings[15];
const char *attribute_strings[3];
const char *shader_file_paths[15];

#endif
