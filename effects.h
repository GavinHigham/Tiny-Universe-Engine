//AUTO-GENERATED FILE, CHANGES MAY BE OVERWRITTEN.

#ifndef EFFECTS_H
#define EFFECTS_H

#include <GL/glew.h>
typedef struct effect_data {
	GLuint handle;
	union {
		struct {
			GLint ambient_pass;
			GLint bpos_size;
			GLint camera_position;
			GLint eye_block_offset;
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
		GLint unif[22];
	};
	union {
		struct {
			GLint sector_coords;
			GLint star_pos;
			GLint vColor;
			GLint vNormal;
			GLint vPos;
		};
		GLint attr[5];
	};
} EFFECT;

union effect_list {
	struct {
		EFFECT debug_graphics;
		EFFECT forward;
		EFFECT outline;
		EFFECT shadow;
		EFFECT skybox;
		EFFECT star_blocks;
		EFFECT stars;
	};
	EFFECT all[7];
};

extern union effect_list effects;

extern const char *uniform_strings[22];
extern const char *attribute_strings[5];
extern const char *shader_file_paths[21];

#endif
