//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#include <GL/glew.h>
#include "shaders.h"
const char *lighting_vs_file_path[] = {"shaders/lighting.vs"};
const char *lighting_fs_file_path[] = {"shaders/lighting.fs"};
const char *lighting_vs_source[] = {
"#version 150\n"
"\n"
"uniform mat4 MVM;\n"
"\n"
"in vec3 vPos;\n"
"void main() {\n"
"	gl_Position = MVM * vec4(vPos, 1);\n"
"}\n"
};
const char *lighting_fs_source[] = {
"#version 140\n"
"//Position, color, and normal buffers, in camera space.\n"
"uniform sampler2D gPositionMap;\n"
"uniform sampler2D gColorMap;\n"
"uniform sampler2D gNormalMap;\n"
"uniform vec2 gScreenSize;\n"
"uniform vec3 uLight_pos; //Light position in camera space.\n"
"uniform vec3 uLight_col; //Light color.\n"
"uniform vec4 uLight_attr; //Light attributes. Falloff factors, then intensity.\n"
"\n"
"#define CONSTANT    0\n"
"#define LINEAR      1\n"
"#define EXPONENTIAL 2\n"
"#define INTENSITY   3\n"
"\n"
"out vec4 LFragment;\n"
"\n"
"struct light_fragment {\n"
"	float specular;\n"
"	float diffuse;\n"
"};\n"
"\n"
"light_fragment point_light_fragment(vec3 world_pos, vec3 normal, vec3 light_pos, vec4 attr)\n"
"{\n"
"	light_fragment tmp;\n"
"	float distance = distance(world_pos, light_pos);\n"
"	float attenuation = attr[CONSTANT] + (attr[LINEAR]*distance) + (attr[EXPONENTIAL]*distance*distance);\n"
"	tmp.diffuse = attr[INTENSITY] * clamp(dot(normalize(light_pos-world_pos), normal), 0, 1) / attenuation;\n"
"\n"
"	vec3 h = normalize(normalize(-world_pos) + normalize(light_pos-world_pos));\n"
"	tmp.specular = attr[INTENSITY] * pow(max(0.0, dot(h, normalize(normal))), 32.0) / attenuation;\n"
"	//tmp.specular = 0;\n"
"	return tmp;\n"
"}\n"
"\n"
"float light_source(vec3 hard_coded_light_pos, vec3 world_pos)\n"
"{\n"
"	float distance = distance(hard_coded_light_pos, world_pos);\n"
"	return pow(100/(distance * distance), 32);\n"
"}\n"
"\n"
"void main() {\n"
"	//Grab attribute info from deferred framebuffer.\n"
"	vec2 TexCoord = gl_FragCoord.xy / gScreenSize;\n"
"	vec3 WorldPos = texture(gPositionMap, TexCoord).xyz;\n"
"	vec3 Color = texture(gColorMap, TexCoord).xyz;\n"
"	vec3 Normal = texture(gNormalMap, TexCoord).xyz;\n"
"\n"
"	//float light_vis = 1/distance(lighting_pass_position, uLight.position);\n"
"	vec4 attr = uLight_attr;\n"
"	vec3 light_col = uLight_col;\n"
"	//light_col = vec3(1.0, 0.4, 0.4);\n"
"	//attr[INTENSITY] *= 3;\n"
"	light_fragment p = point_light_fragment(WorldPos, Normal, uLight_pos, attr);\n"
"   	LFragment = vec4(Color*light_col, 1.0) * (p.diffuse + p.specular);\n"
"   	//LFragment = vec4(Normal, 1.0);\n"
"}\n"
};
const GLchar *lighting_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *lighting_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "projection_matrix", "gScreenSize", "uLight_attr", "uLight_col", "uLight_pos", "NMVM", "MVM"};
static const int lighting_attribute_count = sizeof(lighting_attribute_names)/sizeof(lighting_attribute_names[0]);
static const int lighting_uniform_count = sizeof(lighting_uniform_names)/sizeof(lighting_uniform_names[0]);
GLint lighting_attributes[lighting_attribute_count];
GLint lighting_uniforms[lighting_uniform_count];
struct shader_prog lighting_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {0, 0, 0, -1, 0, 0, 0, 0, -1, 0}
};
struct shader_info lighting_info = {
	.vs_source = lighting_vs_source,
	.fs_source = lighting_fs_source,
	.attr_names = lighting_attribute_names,
	.unif_names = lighting_uniform_names,
	.vs_file_path = lighting_vs_file_path,
	.fs_file_path = lighting_fs_file_path
};
const char *deferred_vs_file_path[] = {"shaders/deferred.vs"};
const char *deferred_fs_file_path[] = {"shaders/deferred.fs"};
const char *deferred_vs_source[] = {
"#version 330 \n"
"\n"
"in vec3 vPos; \n"
"in vec3 vColor; \n"
"in vec3 vNormal; \n"
"\n"
"uniform mat4 projection_matrix;\n"
"uniform mat4 MVM;\n"
"uniform mat4 NMVM;\n"
"\n"
"out vec3 fPos;\n"
"out vec3 fColor;\n"
"out vec3 fNormal;\n"
"\n"
"void main()\n"
"{ \n"
"	vec4 new_vertex = MVM * vec4(vPos, 1);\n"
"	gl_Position = projection_matrix * new_vertex;\n"
"	//Pass along position and normal in camera space.\n"
"	fPos = vec3(new_vertex);\n"
"	fNormal = vec3(vec4(vNormal, 0.0) * NMVM);\n"
"	//Pass along color.\n"
"	fColor = vColor;\n"
"}\n"
};
const char *deferred_fs_source[] = {
"#version 330\n"
"\n"
"in vec3 fPos;\n"
"in vec3 fColor;\n"
"in vec3 fNormal; \n"
"\n"
"layout (location = 0) out vec3 WorldPosOut; \n"
"layout (location = 1) out vec3 DiffuseOut; \n"
"layout (location = 2) out vec3 NormalOut; \n"
"\n"
"void main() \n"
"{ \n"
"    WorldPosOut = fPos; \n"
"    DiffuseOut = fColor; \n"
"    NormalOut = normalize(fNormal); \n"
"}\n"
};
const GLchar *deferred_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *deferred_uniform_names[] = {"gPositionMap", "gNormalMap", "gColorMap", "projection_matrix", "gScreenSize", "uLight_attr", "uLight_col", "uLight_pos", "NMVM", "MVM"};
static const int deferred_attribute_count = sizeof(deferred_attribute_names)/sizeof(deferred_attribute_names[0]);
static const int deferred_uniform_count = sizeof(deferred_uniform_names)/sizeof(deferred_uniform_names[0]);
GLint deferred_attributes[deferred_attribute_count];
GLint deferred_uniforms[deferred_uniform_count];
struct shader_prog deferred_program = {
	.handle = 0,
	.attr = {0, 0, 0},
	.unif = {-1, -1, -1, 0, -1, -1, -1, -1, 0, 0}
};
struct shader_info deferred_info = {
	.vs_source = deferred_vs_source,
	.fs_source = deferred_fs_source,
	.attr_names = deferred_attribute_names,
	.unif_names = deferred_uniform_names,
	.vs_file_path = deferred_vs_file_path,
	.fs_file_path = deferred_fs_file_path
};
struct shader_prog *shader_programs[] = {&lighting_program, &deferred_program};
struct shader_info *shader_infos[] = {&lighting_info, &deferred_info};
