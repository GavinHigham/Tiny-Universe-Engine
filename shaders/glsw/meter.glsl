-- vertex.GL33 --

layout(location = 1) in vec2 pos;
layout(location = 2) in vec4 col;
layout(location = 3) in vec2 tx;

uniform vec2 screen_res = vec2(640, 480);

out vec2 position;
out vec4 color;
out vec2 tx_coord;

void main()
{
	position = pos;
	vec2 p = (pos * 2 / screen_res) - 1.0;
	p.y = -p.y;
	color = col;
	tx_coord = tx / vec2(26 * 6, 4 * 12);
	gl_Position = vec4(p, 0, 1);
}

-- fragment.GL33 --

uniform sampler2D font_tex;
uniform bool textured = false;

in vec2 position;
in vec4 color;
in vec2 tx_coord;
out vec4 LFragment;

void main() {
	vec4 tex_color = texture(font_tex, tx_coord);
	if (textured && tex_color.a == 0.0)
		discard;
	LFragment = color;
}