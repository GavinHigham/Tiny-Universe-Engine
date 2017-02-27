#version 330 

//in vec3 sector_pos; //Position relative to sector origin.
in ivec3 sector_coords;

uniform float log_depth_intermediate_factor; //2.0/log(far_plane_dist/near_plane_dist)
uniform float near_plane_dist;

uniform vec3 eye_pos;
uniform ivec3 eye_sector_coords;

uniform float sector_size;
//uniform float stars_radius;
uniform mat4 model_view_projection_matrix;

out vec3 fpos;

//Copied from space_sector.c
vec3 star_in_eye_space(ivec3 camera_sector, ivec3 sector, vec3 pos)
{
	//Local position + (sector displacement on each axis) * (size of a sector)
	return pos + (sector - camera_sector) * sector_size;	
}

void main()
{
	vec3 sector_pos = vec3(0);
	vec3 vpos = star_in_eye_space(eye_sector_coords, sector_coords, sector_pos);
	//float star_dist = distance(eye_pos, vpos);
	
	gl_Position = model_view_projection_matrix * vec4(vpos, 1);
    gl_Position.z = log(gl_Position.w/near_plane_dist) * log_depth_intermediate_factor - 1; 
    gl_Position.z *= gl_Position.w;
	gl_PointSize = 1;//(1-(star_dist / (0.7*stars_radius))) + 2;
	fpos = vpos;
}