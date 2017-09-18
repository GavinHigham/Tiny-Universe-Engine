//AUTO-GENERATED FILE, CHANGES MAY BE OVERWRITTEN.

#include <GL/glew.h>
#include "effects.h"

const char *shader_file_paths[] = {
	"shaders/debug_graphics.vs",
	NULL,
	"shaders/debug_graphics.fs",
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
	"shaders/star_box.vs",
	NULL,
	"shaders/star_box.fs",
	"shaders/stars.vs",
	NULL,
	"shaders/stars.fs"
};

const char *uniform_strings[] = {"ambient_pass", "bpos_size", "camera_position", "eye_box_offset", "eye_pos", "eye_sector_coords", "gLightPos", "hella_time", "log_depth_intermediate_factor", "model_matrix", "model_view_normal_matrix", "model_view_projection_matrix", "override_col", "projection_view_matrix", "sector_size", "star_box_size", "sun_color", "sun_direction", "uLight_attr", "uLight_col", "uLight_pos", "uOrigin", "zpass"};
const char *attribute_strings[] = {"sector_coords", "star_pos", "vColor", "vNormal", "vPos"};
union effect_list effects = {{{0}}};

