#version 330 

in vec3 vPos; 

uniform mat4 model_matrix;
uniform mat4 model_view_matrix;
uniform mat4 model_view_normal_matrix;
uniform mat4 projection_matrix;

out vec3 fPos;
out vec3 fColor;
out vec3 fNormal;

void main()
{
	gl_Position = projection_matrix * (model_view_matrix * vec4(vPos, 1));
	fPos = vec3(model_matrix * vec4(vPos, 1));
	fColor = vec3(0.2, 0.7, 0.3);
	fNormal = vec3(vec4(vNormal, 0.0) * model_view_normal_matrix);
}