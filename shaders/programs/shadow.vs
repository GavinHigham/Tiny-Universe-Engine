#version 330 

in vec3 vPos; 
uniform mat4 model_matrix;
out vec3 gPos;


void main()
{
	gPos = vec3(model_matrix * vec4(vPos, 1));
}