#version 330 

in vec3 star_pos;
uniform vec3 eye_box_offset;
uniform float bpos_size;
uniform float star_box_size;
uniform float log_depth_intermediate_factor;
uniform mat4 model_view_projection_matrix;

out vec3 fpos;
out vec3 fcol;

void main()
{
	vec3 vpos = star_pos*bpos_size + eye_box_offset;
	float alpha = 1 - pow(length(vpos)/star_box_size/bpos_size, 8);
	
	gl_Position = model_view_projection_matrix * vec4(vpos, 1);
	gl_Position.z = (log2(max(1e-6, gl_Position.z + 1.0)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	//gl_PointSize = 10;//(1-(star_dist / (0.7*stars_radius))) + 2;
	gl_PointSize = 5;
	vec3 teal = vec3(0, 0.9, 0.7);
	vec3 blue = vec3(0, 0.1, 0.98);
	vec3 white = vec3(1.0);
	fcol = (
		0.5*(1 + sin(star_pos.x)) * teal + 
		0.5*(1 + sin(star_pos.y)) * blue +
		0.5*(1 + sin(star_pos.z)) * white) / 2.1;
	fcol *= alpha;

	fpos = vpos;
}