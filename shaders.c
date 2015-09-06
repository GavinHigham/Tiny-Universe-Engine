//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#include <GL/glew.h>
#include "shaders.h"
const char *lighting_vs_source[] = {
"#version 150\n"
"\n"
"uniform mat4 projection_matrix;\n"
"uniform mat4 MVM;\n"
"\n"
"in vec3 vPos;\n"
"void main() {\n"
"	gl_Position = projection_matrix * MVM * vec4(vPos, 1);\n"
"}\n"
};
const char *lighting_fs_source[] = {
"#version 140\n"
"uniform vec3 light_pos;\n"
"out vec4 LFragment;\n"
"\n"
"float point_light(vec3 world_pos, vec3 normal)\n"
"{\n"
"	float intensity = 150;\n"
"	float distance = distance(world_pos, light_pos);\n"
"	float diffuse = intensity * clamp(dot(normalize(light_pos-world_pos), normal), 0, 1) / (distance*distance);\n"
"\n"
"	vec3 h = normalize(normalize(camPos-world_pos) + normalize(light_pos-world_pos));\n"
"	float specular = pow(max(0.0, dot(h, normalize(normal))), 32.0);\n"
"	float ambient = 0.1;\n"
"	return specular + diffuse + ambient;\n"
"}\n"
"\n"
"void main() {\n"
"   	vec2 TexCoord = CalcTexCoord();\n"
"   	vec3 WorldPos = texture(gPositionMap, TexCoord).xyz;\n"
"   	vec3 Color = texture(gColorMap, TexCoord).xyz;\n"
"   	vec3 Normal = texture(gNormalMap, TexCoord).xyz;\n"
"   	Normal = normalize(Normal);\n"
"\n"
"   	LFragment = vec4(Color, 1.0) * point_light(WorldPos, Normal);\n"
"}\n"
};
const GLchar *lighting_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *lighting_uniform_names[] = {"projection_matrix", "emit", "light_pos", "alpha", "NMVM", "MVM"};
static const int lighting_attribute_count = sizeof(lighting_attribute_names)/sizeof(lighting_attribute_names[0]);
static const int lighting_uniform_count = sizeof(lighting_uniform_names)/sizeof(lighting_uniform_names[0]);
GLint lighting_attributes[lighting_attribute_count];
GLint lighting_uniforms[lighting_uniform_count];
struct shader_prog lighting_program = {
	.handle = 0,
	.attr = {-1, -1, 0},
	.unif = {0, -1, 0, -1, -1, 0}
};
struct shader_info lighting_info = {
	.vs_source = lighting_vs_source,
	.fs_source = lighting_fs_source,
	.attr_names = lighting_attribute_names,
	.unif_names = lighting_uniform_names,
};
const char *deferred_vs_source[] = {
"#version 330 \n"
"\n"
"layout (location = 0) in vec3 vPos; \n"
"layout (location = 1) in vec3 vColor; \n"
"layout (location = 2) in vec3 vNormal; \n"
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
"	gl_Position = projection_matrix * MVM * vec4(vPos, 1);\n"
"	fColor = vColor;\n"
"	//Pass along vertex and camera position in worldspace.\n"
"	fPos = vec3(new_vertex);\n"
"	fNormal = vec3(vec4(vNormal, 0.0) * NMVM);\n"
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
const GLchar *deferred_uniform_names[] = {"projection_matrix", "emit", "light_pos", "alpha", "NMVM", "MVM"};
static const int deferred_attribute_count = sizeof(deferred_attribute_names)/sizeof(deferred_attribute_names[0]);
static const int deferred_uniform_count = sizeof(deferred_uniform_names)/sizeof(deferred_uniform_names[0]);
GLint deferred_attributes[deferred_attribute_count];
GLint deferred_uniforms[deferred_uniform_count];
struct shader_prog deferred_program = {
	.handle = 0,
	.attr = {0, 0, 0},
	.unif = {0, -1, -1, -1, 0, 0}
};
struct shader_info deferred_info = {
	.vs_source = deferred_vs_source,
	.fs_source = deferred_fs_source,
	.attr_names = deferred_attribute_names,
	.unif_names = deferred_uniform_names,
};
const char *simple_vs_source[] = {
"#version 150\n"
"\n"
"uniform mat4 projection_matrix;\n"
"uniform mat4 MVM;\n"
"uniform mat4 NMVM;\n"
"\n"
"in vec3 vPos;\n"
"in vec3 vColor;\n"
"in vec3 vNormal;\n"
"out vec3 fPos;\n"
"out vec3 camPos;\n"
"out vec4 fColor;\n"
"out vec3 fNormal;\n"
"void main() {\n"
"	vec4 new_vertex = MVM * vec4(vPos, 1);\n"
"	gl_Position = projection_matrix * MVM * vec4(vPos, 1);\n"
"	fColor = vec4(vColor, 1.0);\n"
"	//Pass along vertex and camera position in worldspace.\n"
"	fPos = vec3(new_vertex);\n"
"	camPos = vec3(0, 0, 0);\n"
"	fNormal = vec3(vec4(vNormal, 0.0) * NMVM);\n"
"}\n"
};
const char *simple_fs_source[] = {
"#version 140\n"
"uniform vec3 light_pos;\n"
"uniform float emit;\n"
"uniform float alpha;\n"
"out vec4 LFragment;\n"
"in vec3 camPos;\n"
"in vec3 fPos;\n"
"in vec4 fColor;\n"
"in vec3 fNormal;\n"
"\n"
"void main() {\n"
"	float dens = 40;\n"
"	float scal = 3/4;\n"
"	float intensity = 150;\n"
"	float distance = distance(fPos, light_pos);\n"
"	float diffuse = intensity * clamp(dot(normalize(light_pos-fPos), fNormal), 0, 1) / (distance*distance);\n"
"\n"
"	vec3 h = normalize(normalize(camPos-fPos) + normalize(light_pos-fPos));\n"
"	float specular = pow(max(0.0, dot(h, normalize(fNormal))), 32.0);\n"
"	float ambient = 0.1;\n"
"	LFragment = vec4(vec3(fColor) * ((1.0-ambient)*diffuse + ambient + emit) + specular, alpha);\n"
"	//LFragment = vec4(fNormal, fColor.a);\n"
"}\n"
};
const GLchar *simple_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *simple_uniform_names[] = {"projection_matrix", "emit", "light_pos", "alpha", "NMVM", "MVM"};
static const int simple_attribute_count = sizeof(simple_attribute_names)/sizeof(simple_attribute_names[0]);
static const int simple_uniform_count = sizeof(simple_uniform_names)/sizeof(simple_uniform_names[0]);
GLint simple_attributes[simple_attribute_count];
GLint simple_uniforms[simple_uniform_count];
struct shader_prog simple_program = {
	.handle = 0,
	.attr = {0, 0, 0},
	.unif = {0, 0, 0, 0, 0, 0}
};
struct shader_info simple_info = {
	.vs_source = simple_vs_source,
	.fs_source = simple_fs_source,
	.attr_names = simple_attribute_names,
	.unif_names = simple_uniform_names,
};
struct shader_prog *shader_programs[] = {&lighting_program, &deferred_program, &simple_program};
struct shader_info *shader_infos[] = {&lighting_info, &deferred_info, &simple_info};
