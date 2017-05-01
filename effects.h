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
			GLint eye_sector_coords;
			GLint gLightPos;
			GLint hella_time;
			GLint log_depth_intermediate_factor;
			GLint model_matrix;
			GLint model_view_normal_matrix;
			GLint model_view_projection_matrix;
			GLint override_col;
			GLint projection_view_matrix;
			GLint sector_size;
			GLint sun_color;
			GLint sun_direction;
			GLint uLight_attr;
			GLint uLight_col;
			GLint uLight_pos;
			GLint uOrigin;
			GLint zpass;
		};
		GLint unif[20];
	};
	union {
		struct {
			GLint sector_coords;
			GLint vColor;
			GLint vNormal;
			GLint vPos;
		};
		GLint attr[4];
	};
} EFFECT;

union effect_list {
	struct {
		EFFECT debug_graphics;
		EFFECT forward;
		EFFECT outline;
		EFFECT shadow;
		EFFECT skybox;
		EFFECT stars;
	};
	EFFECT all[6];
};

union effect_list effects;

const char *uniform_strings[20];
const char *attribute_strings[4];
const char *shader_file_paths[18];

#endif
