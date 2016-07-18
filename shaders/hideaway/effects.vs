#version 330

in vec3 vPos;
//uniform mat4 model_view_matrix;

void main()
{
	gl_Position = vec4(vPos, 1);
}