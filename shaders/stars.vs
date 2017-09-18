#version 330 

//in vec3 sector_pos; //Position relative to sector origin.
in ivec3 sector_coords;

uniform float log_depth_intermediate_factor;
uniform vec3 eye_pos;
uniform ivec3 eye_sector_coords;

uniform float sector_size;
uniform mat4 model_view_projection_matrix;

out vec3 fpos;
out vec3 fcol;

//TODO: Find a solution that works for 64-bit star locations.
//Copied from space_sector.c
vec3 star_in_eye_space(ivec3 eye_sector, ivec3 sector, vec3 pos)
{
	//Local position + (sector displacement on each axis) * (size of a sector)
	return sector_size * (sector - eye_sector) + pos;	
}

void main()
{
	vec3 sector_pos = vec3(0);
	vec3 vpos = star_in_eye_space(eye_sector_coords, sector_coords, sector_pos);
	//float star_dist = distance(eye_pos, vpos);
	
	gl_Position = model_view_projection_matrix * vec4(vpos, 1);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	//gl_PointSize = 10;//(1-(star_dist / (0.7*stars_radius))) + 2;
	gl_PointSize = 5;
	vec3 teal = vec3(0, 0.9, 0.7);
	vec3 blue = vec3(0, 0.1, 0.98);
	vec3 white = vec3(1.0);
	fcol = (
		0.5*(1 + sin(sector_coords.x)) * teal + 
		0.5*(1 + sin(sector_coords.y)) * blue +
		0.5*(1 + sin(sector_coords.z)) * white) / 2.1;

	fpos = vpos;
}