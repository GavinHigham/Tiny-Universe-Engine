#version 330 

in vec3 vPos; 
in vec3 vColor;
in vec3 vNormal;

out vec3 gPos;

void main()
{
	vec3 refattr = vColor + vNormal;
	gPos = vPos;
}