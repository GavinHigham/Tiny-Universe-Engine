#version 330 

in vec3 vPos;
in vec3 vNormal;
uniform mat4 model_matrix;
out vec3 gPos;
out vec3 gNormal;

void main()
{
	gPos = vec3(model_matrix * vec4(vPos, 1));
	gNormal = vNormal;
}