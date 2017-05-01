#version 330 

in vec3 vPos;

uniform float log_depth_intermediate_factor;

uniform mat4 model_matrix;
uniform mat4 model_view_projection_matrix;
out vec3 fPos;

void main()
{
	fPos = vec3(model_matrix * vec4(vPos, 1));
	gl_Position = model_view_projection_matrix * vec4(vPos, 1);
	gl_Position.z = log2(max(1e-6, 1.0 + gl_Position.w)) * log_depth_intermediate_factor - 1.0; //I'm not sure why, but -2.0 works better for me.
}