#version 330 

in vec3 vPos; 
in vec3 vColor; 
in vec3 vNormal; 

uniform mat4 projection_matrix;
uniform mat4 MVM;
uniform mat4 NMVM;

out vec3 fPos;
out vec3 fColor;
out vec3 fNormal;

void main()
{ 
	vec4 new_vertex = MVM * vec4(vPos, 1);
	gl_Position = projection_matrix * new_vertex;
	//Pass along position and normal in camera space.
	fPos = vec3(new_vertex);
	fNormal = vec3(vec4(vNormal, 0.0) * NMVM);
	//Pass along color.
	fColor = vColor;
}