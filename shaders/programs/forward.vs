#version 330 

in vec3 vColor;
in vec3 vPos; 
in vec3 vNormal; 

uniform mat4 model_matrix;
uniform mat4 model_view_normal_matrix;
uniform mat4 projection_matrix;
uniform mat4 projection_view_matrix;
uniform mat4 model_view_matrix;

out vec3 fPos;
out vec3 fColor;
out vec3 fNormal;

void main()
{
	mat4 ref = model_view_matrix;
	mat4 ref2 = projection_matrix;
	gl_Position = projection_view_matrix * (model_matrix * vec4(vPos, 1));
	fPos = vec3(model_matrix * vec4(vPos, 1));
	fColor = vColor;
	fNormal = vec3(vec4(vNormal, 0.0) * model_view_normal_matrix);
}