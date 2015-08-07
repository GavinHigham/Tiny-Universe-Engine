#version 140

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;
in vec3 vertex_pos;
in vec3 vColor;
out vec4 fColor;
void main() {
	vec4 new_vertex = model_view_matrix * vec4(vertex_pos, 1);
	gl_Position = projection_matrix * new_vertex;
	fColor = vec4(vColor, 1.0);
}