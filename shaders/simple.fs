#version 140
out vec4 LFragment;
in vec4 fColor;
void main() {
	LFragment = fColor;//vec4(1.0, 1.0, 1.0, 1.0);
}