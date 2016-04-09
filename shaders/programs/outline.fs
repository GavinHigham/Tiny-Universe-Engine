#version 330

in vec3 fPos;
in vec3 fColor;
in vec3 fNormal;
out vec4 LFragment;

void main() {
	vec3 refAttr = fPos * fColor * fNormal;
	LFragment = vec4(1.0, 0.0, 0.0, 1.0);
}