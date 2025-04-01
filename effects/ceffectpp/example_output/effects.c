//AUTO-GENERATED FILE, CHANGES MAY BE OVERWRITTEN.

#include <GL/glew.h>
#include "effects.h"

const char *shader_file_paths[] = {
	"shaders/forward.vs",
	NULL,
	"shaders/forward.fs",
	"shaders/outline.vs",
	"shaders/outline.gs",
	"shaders/outline.fs",
	"shaders/shadow.vs",
	"shaders/shadow.gs",
	"shaders/shadow.fs",
	"shaders/skybox.vs",
	NULL,
	"shaders/skybox.fs",
	"shaders/stars.vs",
	NULL,
	"shaders/stars.fs"
};

const char *uniform_strings[] = {"ambient_pass", "camera_position", "eye_pos", "gLightPos", "model_matrix", "model_view_normal_matrix", "model_view_projection_matrix", "projection_view_matrix", "sun_color", "sun_direction", "uLight_attr", "uLight_col", "uLight_pos", "uOrigin", "zpass"};
const char *attribute_strings[] = {"vColor", "vNormal", "vPos"};
union effect_list effects = {{{0}}};

