#version 140
uniform vec3 light_pos;
out vec4 LFragment;

float point_light(vec3 world_pos, vec3 normal)
{
	float intensity = 150;
	float distance = distance(world_pos, light_pos);
	float diffuse = intensity * clamp(dot(normalize(light_pos-world_pos), normal), 0, 1) / (distance*distance);

	vec3 h = normalize(normalize(camPos-world_pos) + normalize(light_pos-world_pos));
	float specular = pow(max(0.0, dot(h, normalize(normal))), 32.0);
	float ambient = 0.1;
	return specular + diffuse + ambient;
}

void main() {
   	vec2 TexCoord = CalcTexCoord();
   	vec3 WorldPos = texture(gPositionMap, TexCoord).xyz;
   	vec3 Color = texture(gColorMap, TexCoord).xyz;
   	vec3 Normal = texture(gNormalMap, TexCoord).xyz;
   	Normal = normalize(Normal);

   	LFragment = vec4(Color, 1.0) * point_light(WorldPos, Normal);
}