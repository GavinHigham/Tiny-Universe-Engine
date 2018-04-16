-- vertex.GL33 --

uniform mat4 model_matrix;
uniform mat4 model_view_projection_matrix;
uniform int rows;
uniform sampler2D diffuse_tx;
uniform float log_depth_intermediate_factor;

const int octaves = 5;

layout(location = 1) in vec3 vpos[3];
layout(location = 4) in vec2 vtx[3];
layout(location = 7) in vec2 vlerp;

out vec3 fposition;
out vec3 fobj_position;
out vec2 ftx;
out vec3 fnormal;

//Value in w, grad in xyz
vec4 fbm(in vec3 seed)
{
	vec4 ret_val = vec4(0);
	float w = 0.5;
	for (int i = 0; i < octaves; i++) {
		float s = pow(2, i);
		float w = pow(w, i);
		ret_val += w * snoise_grad(s*seed);
	}

	return ret_val;
}

void main()
{
	vec3 lpos = mix(vpos[0], vpos[1], vlerp.y);
	vec3 rpos = mix(vpos[0], vpos[2], vlerp.y);
	vec4 test_pos = vec4(mix(lpos, rpos, vlerp.x), 1);
	vec3 x = normalize(mix(lpos, rpos, vlerp.x));
	vec4 val = fbm(x);
	vec3 g = val.xyz;
	float R = 1;
	float s = 0.07;
	vec3 p = (R + s * val.w) * x;
	vec3 h = g - dot(g, x)*x;
	vec3 n = x - s * h;
	fnormal = n;

	vec2 ltx = mix(vtx[0], vtx[1], vlerp.y);
	vec2 rtx = mix(vtx[0], vtx[2], vlerp.y);
	ftx = mix(ltx, rtx, vlerp.x);

	gl_Position = model_view_projection_matrix * test_pos;//vec4(p,1);//vec4(normalize(position) * pow(tex.r-tex.g, 0.03), 1);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	//TODO: Model-view normal matrix
	fposition = vec3(model_matrix * test_pos);//vec4(p, 1));
	fobj_position = test_pos.xyz+vec3(2);//p + vec3(2);
}

-- fragment.GL33 --

uniform sampler2D diffuse_tx;

in vec3 fposition;
in vec3 fobj_position;
in vec2 ftx;
in vec3 fnormal;

out vec4 LFragment;
uniform float tex_scale = 3;

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
	vec3 color = triplanar(diffuse_tx, fobj_position.xyz*tex_scale, normal) * max(dot(normal, vec3(1)), 0.0);

	LFragment = vec4(color, 1.0);
}