//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#include "shaders.h"
#include "shader_program.h"

const char *simple_vs_source[] = {
"#version 140\n"
"\n"
"uniform mat4 projection_matrix;\n"
"uniform mat4 model_view_matrix;\n"
"uniform mat3 normal_model_view_matrix;\n"
"in vec3 vPos;\n"
"in vec3 vColor;\n"
"in vec3 vNormal;\n"
"out vec4 fColor;\n"
"out vec3 fNormal;\n"
"void main() {\n"
"	vec4 new_vertex = model_view_matrix * vec4(vPos, 1);\n"
"	gl_Position = projection_matrix * new_vertex;\n"
"	fColor = vec4(vColor, 1.0);\n"
"	fNormal = normal_model_view_matrix * vNormal;\n"
"}\n"
};
const char *simple_fs_source[] = {
"#version 140\n"
"uniform vec3 sun_light;\n"
"out vec4 LFragment;\n"
"in vec4 fColor;\n"
"in vec3 fNormal;\n"
"\n"
"void main() {\n"
"	float diffuse = dot(normalize(sun_light), fNormal);\n"
"	float roughness = 0.2;\n"
"	float ambient = 0.1;\n"
"	LFragment = vec4((0.8*vec3(fColor)+roughness)*(diffuse + ambient), fColor.a);\n"
"}\n"
};
const GLchar *simple_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *simple_uniform_names[] = {"normal_model_view_matrix", "projection_matrix", "model_view_matrix", "sun_light"};
static const int simple_attribute_count = sizeof(simple_attribute_names)/sizeof(simple_attribute_names[0]);
static const int simple_uniform_count = sizeof(simple_uniform_names)/sizeof(simple_uniform_names[0]);
GLint simple_attributes[simple_attribute_count];
GLint simple_uniforms[simple_uniform_count];
struct shader_prog simple_program = {
	.vs_source = simple_vs_source,
	.fs_source = simple_fs_source,
	.handle = 0,
	.attr_cnt = simple_attribute_count,
	.unif_cnt = simple_uniform_count,
	.attr = simple_attributes,
	.unif = simple_uniforms,
	.attr_names = simple_attribute_names,
	.unif_names = simple_uniform_names,
};
