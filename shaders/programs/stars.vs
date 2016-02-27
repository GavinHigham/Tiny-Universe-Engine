#version 330 

in vec3 vPos; 

uniform mat4 model_view_matrix;
uniform mat4 projection_matrix;

out vec3 fPos;

void main()
{
	gl_Position = projection_matrix * (model_view_matrix * vec4(vPos, 1));
	fPos = vPos;
}