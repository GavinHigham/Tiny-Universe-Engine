#version 150

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;

in vec3 vPos;
in vec3 vColor;
in vec3 vNormal;
out vec4 fColor;
out vec3 fNormal;
void main() {
	mat4 normal_model_view_matrix = model_view_matrix;
	normal_model_view_matrix[3] = vec4(0,0,0,1); //Remove translation
	normal_model_view_matrix = inverse(normal_model_view_matrix);

	vec4 new_vertex = model_view_matrix * vec4(vPos, 1);
	gl_Position = projection_matrix * new_vertex;
	fColor = vec4(vColor, 1.0);
	fNormal = vec3(vec4(vNormal, 0.0) * normal_model_view_matrix); //Multiply from the left produces transpose multiply
}