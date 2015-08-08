#version 140
uniform vec3 sun_light;
out vec4 LFragment;
in vec4 fColor;
in vec3 fNormal;

void main() {
	float diffuse = dot(normalize(sun_light), fNormal);
	float roughness = 0.2;
	float ambient = 0.1;
	LFragment = vec4((0.8*vec3(fColor)+roughness)*(diffuse + ambient), fColor.a);
}