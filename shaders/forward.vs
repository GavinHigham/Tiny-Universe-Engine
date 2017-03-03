#version 330 

in vec3 vColor;
in vec3 vPos; 
in vec3 vNormal;

uniform float log_depth_intermediate_factor; //2.0/log(far_plane_dist/near_plane_dist)
uniform float near_plane_dist;

uniform mat4 model_matrix;
uniform mat4 model_view_normal_matrix;
uniform mat4 model_view_projection_matrix;
uniform float hella_time;

out vec3 fPos;
out vec3 fColor;
out vec3 fNormal;

void main()
{
	//vec3 iPos = vPos;
	//float height = 100 * -sin(distance(vPos, vec3(0, 10000, 0))/150 + 5*hella_time);
	//iPos.y += height;
	gl_Position = model_view_projection_matrix * vec4(vPos, 1);
	gl_Position.z = log(gl_Position.w/near_plane_dist) * log_depth_intermediate_factor - 1; 
	gl_Position.z *= gl_Position.w;
	fPos = vec3(model_matrix * vec4(vPos, 1));
	fColor = vColor;
	fNormal = vec3(vec4(vNormal, 0.0) * model_view_normal_matrix);
}


// void main()
// {
// 	vec4 iPos = model_view_projection_matrix * vec4(vPos, 1);
// 	float height = 100 * -sin(distance(iPos.xyz, vec3(0, 10000, 0))/100 + 10*hella_time);
// 	iPos.y += height;
// 	gl_Position = iPos;
// 	fPos = vec3(model_matrix * iPos);
// 	fColor = vColor;
// 	fNormal = vec3(vec4(vNormal, 0.0) * model_view_normal_matrix);
// }