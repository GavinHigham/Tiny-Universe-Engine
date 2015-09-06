#version 330 

layout (location = 0) in vec3 vPos; 
layout (location = 1) in vec3 vColor; 
layout (location = 2) in vec3 vNormal; 

uniform mat4 projection_matrix;
uniform mat4 MVM;
uniform mat4 NMVM;

out vec3 fPos;
out vec3 fColor;
out vec3 fNormal;

void main()
{ 
	vec4 new_vertex = MVM * vec4(vPos, 1);
	gl_Position = projection_matrix * MVM * vec4(vPos, 1);
	fColor = vColor;
	//Pass along vertex and camera position in worldspace.
	fPos = vec3(new_vertex);
	fNormal = vec3(vec4(vNormal, 0.0) * NMVM);
}