#version 150

uniform mat4 MVM;

in vec3 vPos;
void main() {
	gl_Position = MVM * vec4(vPos, 1);
}