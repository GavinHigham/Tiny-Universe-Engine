#version 140
in vec2 LVertexPos2D;
in vec3 vColor;
out vec4 fColor;
void main() {
	gl_Position = vec4(LVertexPos2D.x, LVertexPos2D.y, 0, 1);
	fColor = vec4(vColor, 1.0);
}