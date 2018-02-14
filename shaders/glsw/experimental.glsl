-- vertex.GL33 --

uniform mat4 model_matrix;
uniform mat4 model_view_projection_matrix;
uniform vec3 vpos[3];
uniform vec2 vtx[3];
uniform int rows;
uniform sampler2D diffuse_tx;

in vec2 vlerp;

out vec3 fposition;
out vec3 fobj_position;
out vec2 ftx;

void main()
{
	vec3 lpos = mix(vpos[0], vpos[1], vlerp.y);
	vec3 rpos = mix(vpos[0], vpos[2], vlerp.y);
	vec3 position = normalize(mix(lpos, rpos, vlerp.x));

	vec2 ltx = mix(vtx[0], vtx[1], vlerp.y);
	vec2 rtx = mix(vtx[0], vtx[2], vlerp.y);
	ftx = mix(ltx, rtx, vlerp.x);

	vec4 tex = vec4(0);
	for (int i = 1; i <= 4; i++)
		tex += texture(diffuse_tx, ftx * i);
	tex /= 4;

	gl_Position = model_view_projection_matrix * vec4(position,1);//vec4(normalize(position) * pow(tex.r-tex.g, 0.03), 1);
	fposition = vec3(model_matrix * vec4(position, 1));
	fobj_position = position;
}

-- fragment.GL33 --

uniform sampler2D diffuse_tx;

in vec3 fposition;
in vec3 fobj_position;
in vec2 ftx;

out vec4 LFragment;

float tex_scale = 3.0;

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
	vec3 normal = normalize(cross(dFdy(fposition.xyz), dFdx(fposition.xyz)));

	// vec4 tex = vec4(0);
	// for (int i = 1; i <= 4; i++)
	// 	tex += texture(diffuse_tx, ftx * i);
	// tex /= 4;

	// vec3 color = tex.xyz; //texture(diffuse_tx, ftx).xyz;// * max(dot(normal, vec3(1)), 0.0);
	vec3 color = triplanar(diffuse_tx, fobj_position.xyz, normal);

	LFragment = vec4(color, 1.0);
}