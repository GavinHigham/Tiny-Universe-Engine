#version 330 

in vec3 vColor;
in vec3 vPos; 
in vec3 vNormal;

uniform float log_depth_intermediate_factor;

uniform mat4 model_matrix;
uniform mat4 model_view_normal_matrix;
uniform mat4 model_view_projection_matrix;
uniform float hella_time;

out vec3 fPos;
out vec3 fColor;
out vec3 fNormal;

void main()
{
	gl_Position = model_view_projection_matrix * vec4(vPos, 1);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	fPos = vec3(model_matrix * vec4(vPos, 1));
	fColor = vColor;
	fNormal = vec3(vec4(vNormal, 0.0) * model_view_normal_matrix);
}