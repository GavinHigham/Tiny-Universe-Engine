#version 140

uniform mat4 projection_matrix;
in vec2 LVertexPos2D;
in vec3 vColor;
out vec4 fColor;
void main() {
	gl_Position = projection_matrix*vec4(LVertexPos2D, -2, 1);
	fColor = vec4(vColor, 1.0);
}