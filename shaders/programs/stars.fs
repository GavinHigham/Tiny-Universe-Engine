#version 330

uniform vec3 camera_position; //Camera position in world space.

in vec3 fPos;
out vec4 LFragment;

void main() {
	// float gamma = 2.2;
	//vec3 final_color = vec3(1.0)*dot(fPos-camera_position, );

	// //Tone mapping.
	// final_color = final_color / (final_color + vec3(1.0));
	// //Gamma correction.
	// final_color = pow(final_color, vec3(1.0 / gamma));
	// LFragment = vec4(final_color, 1.0);
	// //LFragment = vec4(normal, 1.0);
	vec2 point_dist = gl_PointCoord*2 - vec2(1.0);
	float attenuation = (point_dist.x*point_dist.x + point_dist.y*point_dist.y)/sqrt(1/2);
	vec3 refCam = camera_position;
	float blueness = 1-distance(fPos, vec3(0.0))/1000;
	LFragment = vec4(vec3(pow(blueness, 4), blueness*blueness, blueness)*attenuation, 1.0);
}