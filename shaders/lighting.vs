#version 150

uniform mat4 projection_matrix;
uniform mat4 MVM;

in vec3 vPos;
void main() {
	gl_Position = projection_matrix * MVM * vec4(vPos, 1);
}