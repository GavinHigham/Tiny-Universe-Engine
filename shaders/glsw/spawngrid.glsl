-- vertex.GL33 --

layout(location = 1) in vec2 pos;

out vec2 position;

void main()
{
	position = pos;
	gl_Position = vec4(pos, -0.5, 1.0);
}

-- fragment.GL33 --

uniform float time;
uniform float p_and_p; //"Poke and Prod", a variable I stick in as a factor when I want to tweak a value with my sliders to see what happens.
uniform vec2 resolution;
uniform vec2 mouse;

in vec4 fposition;
out vec4 LFragment;

vec2 offsets[] = vec2[](
	vec2(-1.0, -1.0), vec2(0.0, -1.0), vec2(1.0, -1.0),
	vec2(-1.0, 0.0),  vec2(0.0, 0.0),  vec2(1.0, 0.0),
	vec2(-1.0, 1.0),  vec2(0.0, 1.0),  vec2(1.0, 1.0));

//Google's Turbo color map function
vec3 TurboColormap(in float x);
#define ROUND_TO_MULTIPLE(number, factor) (floor((number) / (factor)) * (factor))

float sdBox(vec2 p, vec2 b)
{
	vec2 d = abs(p) - b;
	return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float sdCirc(vec2 p, float r)
{
	return length(p) - r;
}

void main() {
	vec2 uv = 2.0 * gl_FragCoord.xy / resolution - 1.0;
	uv.x *= resolution.x / resolution.y;
	vec2 muv = 2.0 * mouse / resolution - 1.0;
	muv.x = - muv.x;

	// vec3 color = vec3(uv, 1.0);
	float w = p_and_p/2.0;
	float c = sdCirc(uv + muv, w);
	float weight = 0.008;
	float outline = smoothstep(0.0, weight, length(c));

	float g = sdBox(mod(uv, w), vec2(w));
	outline += smoothstep(0.0, weight, length(g));
	vec3 color = vec3(2.0 - outline);

	// float distances[9] = float[](
	// 	0.0, 0.0, 0.0,
	// 	0.0, 0.0, 0.0,
	// 	0.0, 0.0, 0.0);
	for (int i = 0; i < 9; i++) {
		if (sdBox(ROUND_TO_MULTIPLE(muv+offsets[i]*w, 3.0*w)+uv+1.5*w-offsets[i]*(w), vec2(w/2.0)) < 0.0) {
			vec2 oi = offsets[i];
			vec3 tmp = vec3(mod(oi.x+oi.y+2.0, 2.0)/2.0, vec2(oi+2.0)*0.3);
			// vec3 bcolor = TurboColormap(((offsets[i].x + 1) * 4.0 + offsets[i].y+1.0)/10);
			// color += bcolor;
			color += tmp;
			break;
		}
	}
	// vec3 color = TurboColormap(outline);
	LFragment = vec4(color, 1.0);
}