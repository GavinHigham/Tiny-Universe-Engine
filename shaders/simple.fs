#version 140
uniform vec3 sun_light;
out vec4 LFragment;
in vec3 camPos;
in vec3 fPos;
in vec4 fColor;
in vec3 fNormal;

void main() {
	float intensity = 150;
	float distance = distance(fPos, sun_light);
	float diffuse = intensity * clamp(dot(normalize(sun_light-fPos), fNormal), 0, 1) / (distance*distance);

	//vec3 E = normalize(camPos - sun_light);
	//vec3 R = reflect(-sun_light, fNormal);
	vec3 h = normalize(normalize(fPos-camPos) + normalize(sun_light-fPos));
	//float specular = clamp( dot( E,R ), 0, 1);
	float specular = pow(dot(h, fNormal), 64.0);
	float ambient = 0.1;
	LFragment = vec4(vec3(fColor)*((1-ambient)*diffuse + ambient + specular), fColor.a);
	LFragment = vec4(fNormal, fColor.a);
}