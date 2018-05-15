-- vertex.GL33 --

uniform mat4 model_matrix;
uniform mat4 model_view_projection_matrix;
uniform int rows;
uniform sampler2D diffuse_tx;
uniform float log_depth_intermediate_factor;
uniform float planet_radius;
uniform vec3 planet_origin;

const int octaves = 5;

layout(location = 1) in vec3 vpos[3];
layout(location = 4) in vec2 vtx[3];
layout(location = 7) in vec2 vlerp;

out vec3 fposition;
out vec2 ftx;
out vec3 fnormal;
out vec3 surface_position;

//Value in w, grad in xyz
vec4 fbm(in vec3 seed)
{
	vec4 ret_val = vec4(0);
	float w = 0.5;
	for (int i = 0; i < octaves; i++) {
		float s = pow(4, i);
		float w = pow(w, i);
		ret_val += w * snoise_grad(s*seed);
	}

	return ret_val;
}

void main()
{
	//camera-world-space
	vec3 lpos = mix(vpos[0], vpos[1], vlerp.y);
	vec3 rpos = mix(vpos[0], vpos[2], vlerp.y);
	vec3 pos  = mix(lpos, rpos, vlerp.x);

	vec3 x = normalize((pos-planet_origin)/planet_radius);
	vec4 val = fbm(x);
	float R = planet_radius;
	float s = 7000000;
	vec3 g = val.xyz / (R + s * val.w); //gradient
	vec3 p = (R + s * val.w) * x /*Reset position*/ + planet_origin;
	vec3 h = g - dot(g, x)*x;
	vec3 n = x - s * h;
	fnormal = n;

	vec2 ltx = mix(vtx[0], vtx[1], vlerp.y);
	vec2 rtx = mix(vtx[0], vtx[2], vlerp.y);
	ftx = mix(ltx, rtx, vlerp.x);

	gl_Position = model_view_projection_matrix * vec4(p,1);//vec4(normalize(position) * pow(tex.r-tex.g, 0.03), 1);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	//TODO: Model-view normal matrix?
	fposition = vec3(model_matrix * vec4(p,1));
	surface_position = p - planet_origin;
}

-- fragment.GL33 --

uniform sampler2D diffuse_tx;

in vec3 fposition;
in vec2 ftx;
in vec3 fnormal;
in vec3 surface_position;

out vec4 LFragment;
uniform float tex_scale = 700000;
uniform vec3 light_pos;
uniform vec3 light_col;
uniform vec3 eye_pos;

float roughnessValue = 0.8; // 0 : smooth, 1: rough
float F0 = 0.2; // fresnel reflectance at normal incidence
float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)
void point_light_fresnel(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse);
void point_light_phong(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse);


vec3 triplanar(sampler2D tex, vec3 pos, vec3 norm)
{
	vec3 triblend = norm*norm;
	triblend = triblend / (triblend.x + triblend.y + triblend.z);

	vec3 albedo_y = texture(tex, pos.xz).xyz; 
	vec3 albedo_x = texture(tex, pos.zy).xyz; 
	vec3 albedo_z = texture(tex, pos.xy).xyz; 

	return (triblend.y * albedo_y) + (triblend.x * albedo_x) + (triblend.z * albedo_z);
}

void main() {
	vec3 normal = normalize(fnormal);
	//vec3 screen_normal = normalize(cross(dFdy(fposition.xyz), dFdx(fposition.xyz)));
	float specular = 1, diffuse = 1;
	point_light_fresnel(normalize(light_pos - fposition), normalize(eye_pos - fposition), normal, specular, diffuse);
	vec3 color = texture(diffuse_tx, ftx).xyz;
	//vec3 color = triplanar(diffuse_tx, surface_position*tex_scale, normal);
	color = color * light_col * diffuse + light_col * specular;

	//Tone mapping
	//color = color / (color + vec3(1.0));
	//Gamma correction.
	float gamma = 2.2;
	color = pow(color, vec3(1.0 / gamma));
	LFragment = vec4(color*(normal+1.0), 1.0);
}