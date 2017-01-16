#version 330 

in vec3 vPos; 

uniform mat4 model_view_projection_matrix;
uniform vec3 eye_pos;
uniform float stars_radius;
uniform float hella_time;

out vec3 fPos;

float mag(vec3 v)
{
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

void main()
{
	vec3 iPos = vPos;
	//float height = 100 * -sin(distance(vPos, vec3(0, 10000, 0))/150 + 5*hella_time);
	//iPos.y += height;

	vec3 pos_ship_relative = vPos - eye_pos;
	//float star_dist = distance(eye_position, vPos);
	float star_dist = distance(eye_pos, vPos);
	
	// float s = 80;
	// float p = mag(pos_ship_relative) / s;
	// p = s * 1-(pow(1.001, -1*(p*p)));
	// pos_ship_relative = p * normalize(pos_ship_relative) + eye_pos;
	// vec4 pos = vec4(pos_ship_relative, 1);
	
	gl_Position = model_view_projection_matrix * vec4(vPos, 1);
	gl_PointSize = (1-(star_dist / (0.7*stars_radius))) + 2;
	fPos = vPos;
}