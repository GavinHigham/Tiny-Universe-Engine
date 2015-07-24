#include "shaders.h"

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
