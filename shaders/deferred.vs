#version 330 

in vec3 vPos; 
in vec3 vColor; 
in vec3 vNormal; 

uniform mat4 projection_matrix;
uniform mat4 MVM;
uniform mat4 NMVM;

out vec3 fPos;
out vec3 fObjectPos;
out vec3 fColor;
out vec3 fNormal;

void main()
{
	vec4 vertex_view_pos =  MVM * vec4(vPos, 1);
	gl_Position = projection_matrix * vertex_view_pos;
	//Pass along position and normal in camera space.
	fPos = vec3(vertex_view_pos);
	fNormal = vec3(vec4(vNormal, 0.0) * NMVM);
	//Pass along color.
	fColor = vColor;
	//Pass along object-space position.
	fObjectPos = vPos;
}