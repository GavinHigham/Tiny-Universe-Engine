#version 330

in vec3 fpos;
in vec3 fcol;
out vec4 LFragment;

//uniform float stars_radius;

void main() {
	vec2 pd = gl_PointCoord - vec2(0.5);
	float alpha = 1-sqrt(2*(pd.x*pd.x + pd.y*pd.y));
	// vec3 far_color = vec3(0.5, 0.6, 0.9);
	// vec3 near_color = vec3(0.4, 0.88, 0.7);
	// vec3 color_center = vec3(0.0);
	// float dist = distance(fpos, color_center);
	// float color_mix = smoothstep(stars_radius / 3, stars_radius * 3 / 2, dist);
	// vec3 color = mix(near_color, far_color, color_mix);
	// //color = far_color;
	// LFragment = vec4(color, alpha);

	// //vec3 color = vec3(1-x*1.1, 1-x*1.11, 1-x);
	// float b = 1-distance(fpos, vec3(0.0))/(stars_radius);
	// LFragment = vec4((vec3(
	// 	pow(b+0.25, 7.5)-0.05,
	// 	pow(b+.18, 1.8),
	// 	b+0.1
	// 	) + vec3(0.1)), alpha);
	LFragment = vec4(fcol*pow(alpha, 4), 1.0);
	//LFragment = vec4(vec3(attenuation), 1.0);
}