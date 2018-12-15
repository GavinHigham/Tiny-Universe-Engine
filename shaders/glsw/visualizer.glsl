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
uniform vec4 iMouse;
uniform vec2 iTime;
uniform vec4 tweaks = vec4(1.0);
uniform vec4 tweaks2 = vec4(1.0);
uniform sampler2D frequencies;

in vec2 position;
out vec4 LFragment;

const float pi = 3.1415926535;
const float tau = 2.0 * pi;
float numbars = 15.0;

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

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    float d = 3.0*distance(uv.y, 0.5);
    
    float channel = 0.0; //Left channel?
        
    float val = texture(frequencies, vec2(uv.x, channel)).x +
                texture(frequencies, vec2(uv.x - 1.0/512.0, channel)).x +
                texture(frequencies, vec2(uv.x + 1.0/512.0, channel)).x;
    val = max(val/3.0, 1.0/128.0);
    // vec3 graph = color_left_to_right(uv.x) * smoothstep(d, d+1.0/128.0, val);
    float white = smoothstep(d, d+1.0/128.0, val);

    // graph += bloom(uv);
    fragColor = vec4(white, white, white, 1.0);
}

void main() {
    vec2 uv = 2.0 * gl_FragCoord.xy / iResolution - 1.0;
    uv.x *= iResolution.x / iResolution.y;

    mainImage(LFragment, gl_FragCoord.xy);
}