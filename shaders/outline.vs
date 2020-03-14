#version 330 

in vec3 vPos;

uniform mat4 model_matrix;
uniform mat4 model_view_projection_matrix;
uniform mat4 model_view_normal_matrix; //Just have it so the "draw adjacent" function doesn't choke.

out vec3 gPos;

void main()
{
	gl_Position = model_view_projection_matrix * vec4(vPos, 1);
	vec4 something = vec4(0.0);
	something = (model_view_normal_matrix * something) * 0.0;
	gPos = vec3(model_matrix * vec4(vPos, 1)) + something.xyz;
}