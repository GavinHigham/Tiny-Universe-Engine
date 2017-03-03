#version 330 

in vec3 vPos;

uniform float log_depth_intermediate_factor; //2.0/log(far_plane_dist/near_plane_dist)
uniform float near_plane_dist;

uniform mat4 model_matrix;
uniform mat4 model_view_projection_matrix;
out vec3 fPos;

void main()
{
	fPos = vec3(model_matrix * vec4(vPos, 1));
	gl_Position = model_view_projection_matrix * vec4(vPos, 1);
	gl_Position.z = log(gl_Position.w/near_plane_dist) * log_depth_intermediate_factor - 1; 
	gl_Position.z *= gl_Position.w;
}