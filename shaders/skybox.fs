#version 330

uniform vec3 camera_position;
uniform vec3 sun_direction;
uniform vec3 sun_color;

uniform samplerCube accum_cube;
uniform int num_frames_accum = 1;
float gamma = 2.2;

in vec3 fPos;
in vec3 fColor;
out vec4 LFragment;

vec3 sky_color(vec3 v, vec3 s, vec3 c)
{
	float sun = clamp(dot(v, s), 0.0, 1.0); //sun direction, determines brightness
	float sky = clamp(dot(v, vec3(0.0, 1.0, 0.0)) + 1.1, 0.0, 1.0);
	return mix(vec3(0.0), vec3(0.0, 0.0, 1.0)*length(c), sky) + c*pow(sky, 2);
}

void main() {
	float gamma = 2.2;
	vec3 v = normalize(fPos-camera_position); //View vector.
	vec3 sunref = sun_color;
	vec3 final_color = sky_color(v, sun_direction, sun_color);

	//Tone mapping.
	final_color = final_color / (final_color + vec3(1.0));
	//Gamma correction.
	final_color = pow(final_color, vec3(1.0 / gamma));
   	LFragment = vec4(final_color, 1.0);
   	// LFragment = vec4(1.0);
   	// LFragment = vec4(fPos, 1.0);
   	// LFragment = vec4(fColor, 1.0);

   	vec3 color = texture(accum_cube, normalize(fPos)).xyz;

	//Tone mapping
	// color = color / (color + vec3(1.0));
	//Gamma correction
	// color = pow(color, vec3(1.0 / gamma));
	LFragment = vec4(color / float(num_frames_accum), 1.0);
}