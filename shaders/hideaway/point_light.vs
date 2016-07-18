#version 330

uniform mat4 MVM;
uniform mat4 projection_matrix;

in vec3 vPos;
void main() {
	gl_Position = projection_matrix * (MVM * vec4(vPos, 1));
}