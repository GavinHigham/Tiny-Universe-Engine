//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#include "shaders.h"
#include "shader_program.h"

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
"	gl_Position = projection_matrix * new_vertex;\n"
"	fColor = vec4(vColor, 1.0);\n"
"	//Pass along vertex and camera position in worldspace.\n"
"	fPos = vPos;\n"
"	camPos = vec3(0, 0, 0);\n"
"	fNormal = vec3(vec4(vNormal, 0.0) * NMVM);\n"
"}\n"
};
const char *simple_fs_source[] = {
"#version 140\n"
"uniform vec3 sun_light;\n"
"out vec4 LFragment;\n"
"in vec3 camPos;\n"
"in vec3 fPos;\n"
"in vec4 fColor;\n"
"in vec3 fNormal;\n"
"\n"
"void main() {\n"
"	float intensity = 150;\n"
"	float distance = distance(fPos, sun_light);\n"
"	float diffuse = intensity * clamp(dot(normalize(sun_light-fPos), fNormal), 0, 1) / (distance*distance);\n"
"\n"
"	//vec3 E = normalize(camPos - sun_light);\n"
"	//vec3 R = reflect(-sun_light, fNormal);\n"
"	vec3 h = normalize(normalize(fPos-camPos) + normalize(sun_light-fPos));\n"
"	//float specular = clamp( dot( E,R ), 0, 1);\n"
"	float specular = pow(dot(h, fNormal), 64.0);\n"
"	float ambient = 0.1;\n"
"	LFragment = vec4(vec3(fColor)*((1-ambient)*diffuse + ambient + specular), fColor.a);\n"
"	LFragment = vec4(fNormal, fColor.a);\n"
"}\n"
};
const GLchar *simple_attribute_names[] = {"vNormal", "vColor", "vPos"};
const GLchar *simple_uniform_names[] = {"projection_matrix", "NMVM", "sun_light", "MVM"};
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
