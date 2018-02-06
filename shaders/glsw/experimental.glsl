-- vertex.GL33 --

uniform mat4 model_matrix;
uniform mat4 model_view_projection_matrix;
uniform vec3 vpos[3];
uniform vec2 vtx[3];
uniform int rows;

in vec2 vlerp;

out vec4 fposition;
out vec2 ftx;

void main()
{
	vec3 lpos = mix(vpos[0], vpos[1], vlerp.y);
	vec3 rpos = mix(vpos[0], vpos[2], vlerp.y);
	vec3 position = mix(lpos, rpos, vlerp.x);

	vec2 ltx = mix(vtx[0], vtx[1], vlerp.y);
	vec2 rtx = mix(vtx[0], vtx[2], vlerp.y);
	ftx = mix(ltx, rtx, vlerp.x);

	gl_Position = model_view_projection_matrix * vec4(position, 1);
	fposition = model_matrix * vec4(position, 1);
}

-- fragment.GL33 --

uniform sampler2D diffuse_tx;

in vec4 fposition;
in vec2 ftx;

out vec4 LFragment;

void main() {
	vec3 normal = normalize(cross(dFdy(fposition.xyz), dFdx(fposition.xyz)));
	vec3 color = texture(diffuse_tx, ftx).xyz * max(dot(normal, vec3(1)), 0.0);

	LFragment = vec4(color, 1.0);
}