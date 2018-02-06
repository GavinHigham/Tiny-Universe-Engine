-- vertex.GL33 --

in vec3 position; 
in vec2 tx;

uniform mat4 model_matrix;
uniform mat4 model_view_projection_matrix;

out vec4 fposition;
out vec2 ftx;

void main()
{
	gl_Position = model_view_projection_matrix * vec4(position, 1);
	fposition = model_matrix * vec4(position, 1);
	ftx = tx;
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