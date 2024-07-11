-- vertex.GL33 --

in vec3 vPos; 
in vec3 vNormal;
in vec3 vColor;

uniform float log_depth_intermediate_factor;

uniform mat4 model_matrix;
uniform mat4 model_view_normal_matrix;
uniform mat4 model_view_projection_matrix;
uniform float hella_time;

out vec3 fPos;
out vec3 fNormal;
out vec3 fColor;

void main()
{
	gl_Position = model_view_projection_matrix * vec4(vPos, 1);
	gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
	fPos = vec3(model_matrix * vec4(vPos, 1));
	fColor = vColor;
	fNormal = vec3(vec4(vNormal, 0.0) * model_view_normal_matrix);
}

-- fragment.GL33 --

uniform vec3 uLight_pos; //Light position in world space.
uniform vec3 uLight_col; //Light color.
uniform vec4 uLight_attr; //Light attributes. Falloff factors, then intensity.
uniform vec3 camera_position; //Camera position in world space.
uniform int ambient_pass;

uniform vec3 override_col = vec3(1.0, 1.0, 1.0);

#define CONSTANT    0
#define LINEAR      1
#define EXPONENTIAL 2
#define INTENSITY   3
#define M_PI 3.1415926535897932384626433832795

in vec3 fPos;
in vec3 fNormal;
in vec3 fColor;

out vec4 LFragment;

// set important material values
float roughnessValue = 0.8; // 0 : smooth, 1: rough
float F0 = 0.2; // fresnel reflectance at normal incidence
float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)
void point_light_fresnel(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse);

vec3 sky_color(vec3 v, vec3 s, vec3 c)
{
	float sun = clamp(dot(v, s), 0.0, 1.0); //sun direction, determines brightness
	return mix(vec3(0.0), vec3(0.0, 0.0, 1.0)*length(c), sun) + c*pow(sun, 2);
}

void main() {
	float gamma = 2.2;
	vec3 color = fColor;
	//roughnessValue = pow(0.2*length(color), 8);
	color *= override_col;
	vec3 normal = normalize(fNormal);
	vec3 final_color = vec3(0.0);
	float specular, diffuse;
	vec3 diffuse_frag = vec3(0.0);
	vec3 specular_frag = vec3(0.0);
	vec3 v = normalize(camera_position - fPos); //View vector.
	if (ambient_pass == 1) {
		vec3 l = normalize(uLight_pos-fPos);
		point_light_fresnel(l, v, normal, specular, diffuse);
		diffuse_frag = color*diffuse*sky_color(normal, normalize(vec3(0.1, 0.8, 0.1)), uLight_col);
		specular_frag = color*specular*sky_color(reflect(v, normal), normalize(vec3(0.1, 0.8, 0.1)), uLight_col);
		diffuse_frag = color*diffuse*uLight_col;
		specular_frag = color*specular*uLight_col;
	} else {
		float dist = distance(fPos, uLight_pos);
		float attenuation = uLight_attr[CONSTANT] + uLight_attr[LINEAR]*dist + uLight_attr[EXPONENTIAL]*dist*dist;
		vec3 l = normalize(uLight_pos-fPos); //Light vector.
		point_light_fresnel(l, v, normal, specular, diffuse);
		diffuse_frag = color*uLight_col*uLight_attr[INTENSITY]*diffuse/attenuation;
		specular_frag = color*uLight_col*uLight_attr[INTENSITY]*specular/attenuation;
	}

	final_color += (diffuse_frag + specular_frag);
	//vec3 fog_color = vec3(0.0, 0.0, 0.0);
	//final_color = mix(final_color, fog_color, pow(distance(fPos, camera_position)/1000, 4.0)); 

	//Tone mapping.
	//float x = 0.0001;
	//final_color = (final_color * (6.2 * x + 0.5))/(final_color * (6.2 * final_color + 1.7) + 0.06);
	final_color = final_color / (final_color + vec3(1.0));
	//final_color = final_color / (max(max(final_color.x, final_color.y), final_color.z) + 1);

	//Gamma correction.
	final_color = pow(final_color, vec3(1.0 / gamma));
	LFragment = vec4(final_color, 1.0);
}