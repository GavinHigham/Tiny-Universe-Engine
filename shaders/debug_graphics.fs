#version 330

in vec3 fColor;

out vec4 LFragment;

void main() {
	LFragment = vec4(fColor, 1.0);
}