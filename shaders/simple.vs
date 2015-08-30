#version 150

uniform mat4 projection_matrix;
uniform mat4 MVM;
uniform mat4 NMVM;

in vec3 vPos;
in vec3 vColor;
in vec3 vNormal;
out vec3 fPos;
out vec3 camPos;
out vec4 fColor;
out vec3 fNormal;
void main() {
	vec4 new_vertex = MVM * vec4(vPos, 1);
	gl_Position = projection_matrix * new_vertex;
	fColor = vec4(vColor, 1.0);
	//Pass along vertex and camera position in worldspace.
	fPos = vPos;
	camPos = vec3(0, 0, 0);
	fNormal = vec3(vec4(vNormal, 0.0) * NMVM);
}