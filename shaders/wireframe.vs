#version 330 

in vec3 vPos; 

uniform mat4 model_view_projection_matrix;

void main()
{
	gl_Position = model_view_projection_matrix * vec4(vPos, 1);
}