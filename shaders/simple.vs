#version 140

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;
uniform mat3 normal_model_view_matrix;
in vec3 vPos;
in vec3 vColor;
in vec3 vNormal;
out vec4 fColor;
out vec3 fNormal;
void main() {
	vec4 new_vertex = model_view_matrix * vec4(vPos, 1);
	gl_Position = projection_matrix * new_vertex;
	fColor = vec4(vColor, 1.0);
	fNormal = normal_model_view_matrix * vNormal;
}