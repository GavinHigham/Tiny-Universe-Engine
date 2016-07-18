#version 330
//Position, color, and normal buffers, in camera space.
uniform sampler2D diffuse_light;
uniform sampler2D specular_light;
uniform vec2 gScreenSize;

out vec4 LFragment;

void main() {
	float gamma = 2.2;

	vec2 TexCoord = gl_FragCoord.xy / gScreenSize;
	vec3 diffuse_frag = texture(diffuse_light, TexCoord).xyz;
	vec3 specular_frag = texture(specular_light, TexCoord).xyz;

	vec3 final_color = diffuse_frag + specular_frag;
	//Tone mapping.
	final_color = final_color / (final_color + vec3(1.0));
	//Gamma correction.
	final_color = pow(final_color, vec3(1.0 / gamma));
	//LFragment = vec4(vec3(pow(gl_FragCoord.z, 2)), 1.0);
   	LFragment = vec4(final_color, 1.0);
}