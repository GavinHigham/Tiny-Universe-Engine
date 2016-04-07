#version 140
uniform vec3 uLight_col; //Light color.
out vec4 LFragment;

void main() {
   	LFragment = vec4(uLight_col, 1);
}