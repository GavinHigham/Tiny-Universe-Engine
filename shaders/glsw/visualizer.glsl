-- vertex.GL33 --

layout(location = 1) in vec2 pos;

out vec2 position;

void main()
{
	position = pos;
	gl_Position = vec4(pos, -0.5, 1);
}

-- fragment.GL33 --

uniform vec2 iResolution;
uniform vec2 iTime;
uniform vec4 tweaks = vec4(1.0);
uniform vec4 tweaks2 = vec4(1.0);
uniform sampler2D frequencies;
uniform int style = 0;

in vec2 position;
out vec4 LFragment;

const float pi = 3.1415926535;
const float tau = 2.0 * pi;
float numbars = 15.0;

float sdLine( in vec2 p, in vec2 a, in vec2 b )
{
	vec2 pa = p-a, ba = b-a;
	float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
	return length( pa - ba*h );
}

float sdBox( in vec2 p, in vec2 b )
{
	vec2 d = abs(p)-b;
	return length(max(d,vec2(0))) + min(max(d.x,d.y),0.0);
}

float fsu(float a)
{
	return (a + 1.0)*0.5;
}

vec3 color_left_to_right(float a)
{
	float b = (1.0-a) * 3.0 + 1.8;
	return vec3(fsu(sin(b)), fsu(sin(b + tau/3.0)), fsu(sin(b + tau*(2.0/3.0))));
}

vec3 bloom(vec2 uv)
{
	vec2 uvp = mix(uv, vec2(0.5), 0.2);
	return mix(
		color_left_to_right(uv.x)  * pow(1.0 - 1.2*distance(uv,  vec2(0.5)), 2.0),
		color_left_to_right(uvp.x) * pow(1.0 - 1.3*distance(uvp, vec2(0.5)), 2.0),
		0.5);
}

// void mainImage( out vec4 fragColor, in vec2 fragCoord )
// {
//     // Normalized pixel coordinates (from 0 to 1)
//     vec2 uv = fragCoord/iResolution.xy;
//     float d = 3.0*distance(uv.y, 0.5);
	
//     float channel = 0.0; //Left channel?
		
//     float val = texture(frequencies, vec2(uv.x, channel)).x +
//                 texture(frequencies, vec2(uv.x - 1.0/512.0, channel)).x +
//                 texture(frequencies, vec2(uv.x + 1.0/512.0, channel)).x;
//     val = max(val/3.0, 1.0/128.0);
//     // vec3 graph = color_left_to_right(uv.x) * smoothstep(d, d+1.0/128.0, val);
//     float white = smoothstep(d, d+1.0/128.0, val);

//     // graph += bloom(uv);
//     fragColor = vec4(white, white, white, 1.0);
// }

float roundToMultiple(float number, float factor)
{
	return floor(number / factor) * factor;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	float num_buckets = tweaks[0];
	float bar_width = tweaks[1];
	float bar_spacing = tweaks[2];
	float bar_min_height = tweaks[3];
	float bar_width_total = bar_width + bar_spacing;
	float inner_radius = tweaks2[0] / iResolution.y;
	float circle_rotation = tweaks2[1];
	float circle_fraction = tweaks2[2];
	float bar_power = tweaks2[3];

	// Normalized pixel coordinates (from 0 to 1)
	vec2 uv = fragCoord/iResolution.xy;
	float channel = 0.0; //Left channel?

	if (style == 0.0 || style == 1.0) {
		fragCoord.x -= (iResolution.x - bar_width_total * num_buckets) / 2.0;
		float uvx = fragCoord.x / (bar_width_total * num_buckets);
		float uvx_floor = floor(uvx * num_buckets) / num_buckets;
		
		// float val = texture(frequencies, vec2(uv_floor.x, channel)).x;
		float val = texture(frequencies, vec2(uvx_floor, channel)).x;
		val = pow(val, bar_power);
		// // vec3 graph = color_left_to_right(uv.x) * smoothstep(d, d+1.0/128.0, val);
		// float white = 0.0;//step(d, d+1.0/128.0, val);
		// if (val > d && distance(uv.x, uv_floor.x) * iResolution.x < bar_width && uv.y > 0.5)
		if (val > uv.y - bar_min_height / iResolution.y &&
			fragCoord.x < (bar_width_total * num_buckets) &&
			fragCoord.x > 0.0 &&
			distance(fragCoord.x, roundToMultiple(fragCoord.x, bar_width_total)) < bar_width) {
			fragColor = vec4(1.0);
			if (style == 1.0)
				fragColor *= vec4(color_left_to_right(uv.x), 1.0);
		// else if (uv.y > 0.5)
		//     fragColor = vec4(0.0, uvx_floor, 0.0, 1.0);
		// else if (uv.y < 0.5)
		//     fragColor = vec4(0.0, uvx, 0.0, 1.0);
		} else {
			fragColor = vec4(0.0, 0.0, 0.0, 1.0);
		}
	} else {
		//Displacement from center
		vec2 p = uv - 0.5;
		//parameter running from 0 to 1 along the frequency spectrum
		float t = mod(atan(p.y, p.x) / tau + circle_rotation, 1) / circle_fraction;
		float t_floor = floor(t * num_buckets) / num_buckets;
		float val = texture(frequencies, vec2(t_floor, channel)).x;
		val = pow(val, bar_power);

			// fragCoord.x < (bar_width_total * num_buckets) &&
		if (val + bar_min_height / iResolution.y > length(p) - inner_radius &&
			distance(t, roundToMultiple(t, 1.0 / num_buckets)) < 1/num_buckets/bar_spacing &&
			t <= 1.0 &&
			length(p) > inner_radius) {
			fragColor = vec4(1.0);
			if (style == 3.0)
				fragColor *= vec4(color_left_to_right(t), 1.0);
		}
		else
			fragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
	// float white = val / 128.0;

	// graph += bloom(uv);
}

#define SAMPLES_PER_AXIS 4
void main() {
	vec2 uv = 2.0 * gl_FragCoord.xy / iResolution - 1.0;
	uv.x *= iResolution.x / iResolution.y;
	if (style > 1.0) { //Circular styles need anti-aliasing
		vec4 sum = vec4(0.0);
		vec4 sample;
		for (int i = 0; i < SAMPLES_PER_AXIS; i++) {
			for (int j = 0; j < SAMPLES_PER_AXIS; j++) {
				mainImage(sample, gl_FragCoord.xy + vec2(float(i)/SAMPLES_PER_AXIS, float(j)/SAMPLES_PER_AXIS));
				sum += sample;
			}
		}
		LFragment = sum / (SAMPLES_PER_AXIS * SAMPLES_PER_AXIS);
	} else {
		mainImage(LFragment, gl_FragCoord.xy);
	}
}