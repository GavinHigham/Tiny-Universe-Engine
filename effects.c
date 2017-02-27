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

const char *uniform_strings[] = {"ambient_pass", "camera_position", "eye_pos", "eye_sector_coords", "gLightPos", "hella_time", "log_depth_intermediate_factor", "model_matrix", "model_view_normal_matrix", "model_view_projection_matrix", "near_plane_dist", "projection_view_matrix", "sector_size", "sun_color", "sun_direction", "uLight_attr", "uLight_col", "uLight_pos", "uOrigin", "zpass"};
const char *attribute_strings[] = {"sector_coords", "vColor", "vNormal", "vPos"};
union effect_list effects = {{{0}}};
