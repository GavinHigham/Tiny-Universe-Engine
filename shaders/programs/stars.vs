#version 330 

in vec3 vPos; 

uniform mat4 model_view_matrix;
uniform mat4 projection_matrix;
uniform vec3 eye_pos;

out vec3 fPos;

float mag(vec3 v)
{
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

void main()
{
	vec3 pos_ship_relative = vPos - eye_pos;
	//float star_dist = distance(eye_position, vPos);
	float star_dist = distance(eye_pos, vPos);
	/*
	float s = 80;
	float p = mag(pos_ship_relative) / s;
	p = s * 1-(pow(1.001, -1*(p*p)));
	pos_ship_relative = p * normalize(pos_ship_relative) + ship_position;
	vec4 pos = vec4(pos_ship_relative, 1);
	*/
	vec4 pos = vec4(vPos, 1);
	gl_Position = projection_matrix * (model_view_matrix * pos);
	gl_PointSize = 1;//(1-(star_dist / 700)) + 2;
	fPos = pos.xyz;
}