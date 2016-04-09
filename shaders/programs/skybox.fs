#version 330

uniform vec3 camera_position;

in vec3 fPos;
out vec4 LFragment;

void main() {
	vec3 sun_dir = normalize(vec3(0, 0, -1));
	float gamma = 2.2;
	vec3 v = normalize(camera_position-fPos); //View vector.
	float sky = dot(v, vec3(0, 1, 0)); //sky direction, determines whiteness
	float sun = dot(v, sun_dir); //sun direction, determines brightness
	vec3 ambient = vec3(0.0, 0.05, 0.2); //minimum amount of light
	vec3 sky_color = sun*vec3(0.1, 0.1, 1) + ambient;
	//vec3 sky_color = 4*s*vec3(0.1,0.3,0.8)+ambient;//*vec3(.6)+vec3(0.1,0.3,0.8);

	vec3 final_color = sky_color;

	//Tone mapping.
	final_color = final_color / (final_color + vec3(1.0));
	//Gamma correction.
	final_color = pow(final_color, vec3(1.0 / gamma));
   	LFragment = vec4(final_color, 1.0);
   	//LFragment = vec4(fColor, 1.0);
}