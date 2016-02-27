#version 330 

in vec3 vColor;
in vec3 vPos; 
in vec3 vNormal; 

uniform mat4 model_matrix;
uniform mat4 model_view_matrix;
uniform mat4 model_view_normal_matrix;
uniform mat4 projection_matrix;

out vec3 gPos;
out vec3 gColor;
out vec3 gNormal;

void main()
{
	gl_Position = projection_matrix * (model_view_matrix * vec4(vPos, 1));
	gPos = vec3(model_matrix * vec4(vPos, 1));
	gColor = vColor;
	gNormal = vec3(vec4(vNormal, 0.0) * model_view_normal_matrix);
}