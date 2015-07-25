#include "shaders.h"
#include "shader_program.h"

const char *simple_vs_source[] = {
"#version 140\n"
"in vec2 LVertexPos2D;\n"
"void main() {\n"
"	gl_Position = vec4(LVertexPos2D.x, LVertexPos2D.y, 0, 1);\n"
"}\n"
};
const char *simple_fs_source[] = {
"#version 140\n"
"out vec4 LFragment;\n"
"void main() {\n"
"	LFragment = vec4(1.0, 1.0, 1.0, 1.0);\n"
"}\n"
};

const GLchar *simple_attribute_names[] = {"LVertexPos2D"};
static const int simple_attribute_count = sizeof(simple_attribute_names)/sizeof(simple_attribute_names[0]);
GLint simple_attributes[simple_attribute_count];
struct shader_prog simple_program = {
	.vs_source = simple_vs_source,
	.fs_source = simple_fs_source,
	.handle = 0,
	.attr_cnt = simple_attribute_count,
	.attr = simple_attributes,
	.attr_names = simple_attribute_names,
};
