#version 150

uniform mat4 MVM;
uniform mat4 projection_matrix;

in vec3 vPos;
void main() {
	vec4 vPos_camera_space = MVM * vec4(vPos, 1);
	gl_Position = projection_matrix * vPos_camera_space;
}