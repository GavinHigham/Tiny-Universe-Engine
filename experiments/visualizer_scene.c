#include "luaengine/lua_configuration.h"
#include "deferred_framebuffer.h"
#include "drawf.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "input_event.h"
#include "kiss_fftr.h"
#include "macros.h"
#include "math/utility.h"
#include "meter/meter.h"
#include "meter/meter_ogl_renderer.h"
#include "scene.h"
#include "shader_utils.h"
#include "visualizer_scene.h"

#include <assert.h>
#include <GL/glew.h>
#include <glla.h>
#include <stdio.h>

/* Implementing scene "interface" */

SCENE_IMPLEMENT(visualizer);

static float screen_width = 640, screen_height = 480;
static float obuffer_width = 1920, obuffer_height = 1080;
static int mouse_x = 0, mouse_y = 0;
static struct color_buffer obuffer;
static int accum_frames = 0;
static float tweaks[8] = {1, 1, 1, 1, 1, 1, 1, 1};

#define UNIFORM(name, decl) GLuint name;
#define ATTRIBUTE(name) GLuint name;
struct visualizer_ogl {
	GLuint shader, vao, vbo;
	//Generate OpenGL handles for all uniforms and attributes.
	#include "experiments/visualizer_glsl_bp.h"
} g_visualizer_ogl;
#undef UNIFORM
#undef ATTRIBUTE

#define UNIFORM(name, decl) decl;
struct visualizer_tweaks {
	//Generate storage for uniform variable values.
	#include "experiments/visualizer_glsl_bp.h"
} g_visualizer_tweaks;
#undef UNIFORM

/* Recording file */
FILE *visualizer_file;
int *visualizer_file_buffer = NULL;
static bool visualizer_recording = false;

/* Lua Config */
extern lua_State *L;

/* OpenGL Variables */

// static GLint g_visualizer_ogl.pos = 1;
// static struct {
// 	GLuint RESOLUTION, MOUSE, TIME, TWEAKS, TWEAKS2, TEX, STYLE;
// } UNIF;
// static GLuint SHADER, g_visualizer_ogl.vao, g_visualizer_ogl.vbo;
static GLfloat vertices[] = {-1, -1, 1, -1, -1, 1, 1, 1};

/* Texture and audio */
GLenum tex_fmt = GL_RG8UI;
const int g_tex_w = 2048, g_tex_h = 2, g_texel_size = 2;
char g_tex_buf[g_tex_w*g_tex_h*g_texel_size] = {0};
unsigned char *g_wav_buffer = NULL;
uint32_t g_wav_length;

SDL_AudioSpec g_wav_spec = {0}, g_audio_spec = {0};
SDL_AudioDeviceID g_dev_id;
int nfft = 16384;//4096; //2^14
kiss_fftr_cfg cfg;
kiss_fft_scalar *g_timedata;
kiss_fft_cpx    *g_freqdata;
float g_num_buckets, g_db_multiplier, g_db_divisor, g_blank_seconds;

enum viz_style {VIZ_STYLE_BAR, VIZ_STYLE_BAR_COLOR, VIZ_STYLE_CIRCLE, VIZ_STYLE_CIRCLE_COLOR, NUM_VIZ_STYLES};
enum viz_style g_viz_style = VIZ_STYLE_BAR;

struct viz_circle {
	float inner_radius;
	float rotation;
	bool flip;
} g_viz_circle = {
	.inner_radius = 3.0,
	.rotation = 0.0,
	.flip = false,
};

/* UI */
static bool show_tweaks = true;
static bool visuals_overflowing = false;
static int g_offset_x = 0, g_offset_y = 0;
static float g_y_offset;
static meter_ctx g_viz_meters;

static float hann(int n, int N)
{
	float s = sin((M_PI*n)/(N-1));
	return s*s;
}

static bool bar_overflowing()
{
	float bar_width = tweaks[1];
	float bar_spacing = tweaks[2];
	float num_buckets = g_num_buckets;
	return (bar_width + bar_spacing) * num_buckets > obuffer_width;
}

static void visualizer_meter_callback(char *name, enum meter_state state, float value, void *context)
{
	visuals_overflowing = bar_overflowing() && ((g_viz_style == VIZ_STYLE_BAR) || (g_viz_style == VIZ_STYLE_BAR_COLOR));
}

void init_viz_meters(meter_ctx *M, widget_meter *widgets, int num_widgets, float *y_offset)
{
	for (int i = 0; i < num_widgets; i++) {
		widget_meter *w = &widgets[i];
		meter_add(M, w->name, w->style.width, w->style.height, w->min, w->value, w->max);
		meter_target(M, w->name, w->target);
		meter_position(M, w->name, w->x, w->y + *y_offset);
		meter_callback(M, w->name, w->callback, w->callback_context);
		meter_style(M, w->name, w->color.fill, w->color.border, w->color.font, w->style.padding, 0);
		meter_always_snap(M, w->name, w->always_snap);
		*y_offset += 25;
	}
}

static void set_bar_mode(enum viz_style s)
{
	g_viz_style = s;
	obuffer_width = getglob(L, "recording_width", 1920);
	obuffer_height = getglob(L, "recording_height", 1080);
	color_buffer_delete(obuffer);
	obuffer = color_buffer_new(obuffer_width, obuffer_height);
	visuals_overflowing = bar_overflowing() && g_viz_style == ((g_viz_style == VIZ_STYLE_BAR) || (g_viz_style == VIZ_STYLE_BAR_COLOR));
}

static void set_circle_mode(enum viz_style s)
{
	g_viz_style = s;
	obuffer_width = obuffer_height = getglob(L, "visualizer_circle_width", 1024);
	color_buffer_delete(obuffer);
	obuffer = color_buffer_new(obuffer_width, obuffer_height);
	visuals_overflowing = false;
}

static void visualizer_style_callback(char *name, enum meter_state state, float value, void *context)
{
	// visualizer_meter_callback(name, state, value, context);
	// float bar_width = tweaks[1];
	// float bar_spacing = tweaks[2];
	// float num_buckets = g_num_buckets;
	// if ((bar_width + bar_spacing) * num_buckets > obuffer_width)
	// 	visuals_overflowing = true;
	// else
	// 	visuals_overflowing = false;

	// meter_label(name, )
	value = round(value);
	enum viz_style s = value;
	meter_raw_set(&g_viz_meters, name, value);

	if (g_viz_style != s) {
		switch(s) {
		case VIZ_STYLE_BAR: //fall-through
		case VIZ_STYLE_BAR_COLOR: set_bar_mode(s); break;
		case VIZ_STYLE_CIRCLE: //fall-through
		case VIZ_STYLE_CIRCLE_COLOR: set_circle_mode(s); break;
		default: break;
		}
	}
}

void rem_viz_meters(widget_meter *widgets, int num_widgets, float *y_offset)
{
	for (int i = 0; i < num_widgets; i++)
		meter_delete(&g_viz_meters, widgets[i].name);
	*y_offset -= num_widgets * 25;
}

int visualizer_scene_init()
{
	glGenVertexArrays(1, &g_visualizer_ogl.vao);
	glBindVertexArray(g_visualizer_ogl.vao);

	/* Shader initialization */
	lua_getglobal(L, "data_path");
	lua_pushstring(L, "shaders/glsw/");
	lua_concat(L, 2);
	const char *path = lua_tostring(L, -1);

	glswInit();
	glswSetPath(path, ".glsl");
	glswAddDirectiveToken("glsl330", "#version 330");
	lua_pop(L, 1);

	char *vsh_key = getglobstr(L, "visualizer_vsh_key", "");
	char *fsh_key = getglobstr(L, "visualizer_fsh_key", "");

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "versions.glsl330", vsh_key),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "versions.glsl330", fsh_key),
	};
	g_visualizer_ogl.shader = glsw_new_shader_program(shader, LENGTH(shader));
	glswShutdown();
	free(vsh_key);
	free(fsh_key);

	if (!g_visualizer_ogl.shader) {
		visualizer_scene_deinit();
		return -1;
	}

	/* Retrieve uniform variable handles */

	// g_visualizer_ogl.iResolution  = glGetUniformLocation(SHADER, "iResolution");
	// g_visualizer_ogl.iMouse       = glGetUniformLocation(SHADER, "iMouse");
	// g_visualizer_ogl.iTime        = glGetUniformLocation(SHADER, "iTime");
	// g_visualizer_ogl.tweaks      = glGetUniformLocation(SHADER, "tweaks");
	// g_visualizer_ogl.tweaks2     = glGetUniformLocation(SHADER, "tweaks2");
	// g_visualizer_ogl.frequencies         = glGetUniformLocation(SHADER, "frequencies");
	// g_visualizer_ogl.style       = glGetUniformLocation(SHADER, "style");

	// g_visualizer_ogl.shader = SHADER;

	#define UNIFORM(name, ...) g_visualizer_ogl.name = glGetUniformLocation(g_visualizer_ogl.shader, #name);
	#include "visualizer_glsl_bp.h"
	#undef UNIFORM

	checkErrors("After getting uniform handles");
	glUseProgram(g_visualizer_ogl.shader);

	/* Vertex data */

	glGenBuffers(1, &g_visualizer_ogl.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, g_visualizer_ogl.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	/* Vertex attributes */

	g_visualizer_ogl.pos = glGetAttribLocation(g_visualizer_ogl.shader, "pos");
	glEnableVertexAttribArray(g_visualizer_ogl.pos);
	glVertexAttribPointer(g_visualizer_ogl.pos, 2, GL_FLOAT, GL_FALSE, 0, false);
	checkErrors("After pos attr");

	/* Audio */
	char *wav_file = getglobstr(L, "wav_filename", "");

	/* Load the WAV */
	if (SDL_LoadWAV(wav_file, &g_wav_spec, &g_wav_buffer, &g_wav_length) == NULL) {
		fprintf(stderr, "Could not open %s: %s\n", wav_file, SDL_GetError());
	} else {
		/* Do stuff with the WAV data, and then... */
		printf("freq: %d, fmt: %x, channels: %d, samples: %d, length %d\n", g_wav_spec.freq, g_wav_spec.format, g_wav_spec.channels, g_wav_spec.samples, g_wav_length);

		// SDL_AudioSpec want;
		SDL_AudioSpec have;
		SDL_AudioDeviceID dev;

		// SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
		// want.freq = 48000;
		// want.format = AUDIO_F32;
		// want.channels = 2;
		// want.samples = 4096;
		// want.callback = NULL; /* you wrote this function elsewhere -- see SDL_AudioSpec for details */

		dev = SDL_OpenAudioDevice(NULL, 0, &g_wav_spec, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
		g_dev_id = dev;
		if (dev == 0) {
			SDL_Log("Failed to open audio: %s", SDL_GetError());
		} else {
			if (have.format != g_wav_spec.format) { /* we let this one thing change. */
				SDL_Log("We didn't get requested audio format.");
			}
			SDL_QueueAudio(dev, g_wav_buffer, g_wav_length);
			SDL_PauseAudioDevice(dev, 0); /* start audio playing. */
			// SDL_Delay(5000); /* let the audio callback play some sound for 5 seconds. */
			// SDL_CloseAudioDevice(dev);
		}
	}
	free(wav_file);

	cfg = kiss_fftr_alloc(nfft, 0, NULL, NULL);
	g_timedata = malloc(nfft*sizeof(kiss_fft_scalar));
	g_freqdata = malloc(nfft*sizeof(kiss_fft_cpx));


	/* Texture creation */

	glGenTextures(1, &g_visualizer_tweaks.frequencies);
	glBindTexture(GL_TEXTURE_2D, g_visualizer_tweaks.frequencies);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, g_tex_w, g_tex_h, 0, GL_RED, GL_UNSIGNED_BYTE, g_tex_buf);

	/* Visualizer Style */
	g_viz_style = getglob(L, "visualizer_style", 0);

	/* Offscreen Buffer */
	obuffer_width = getglob(L, "recording_width", 1920);
	obuffer_height = getglob(L, "recording_height", 1080);
	// g_offset_x = getglob(L, "visualizer_offset_x",                 fmax((screen_width  - obuffer_width)  / 2, 0));
	// g_offset_y = getglob(L, "visualizer_offset_y", screen_height - fmax((screen_height - obuffer_height) / 2, 0));
	obuffer = color_buffer_new(obuffer_width, obuffer_height);

	/* Misc. OpenGL bits */

	glClearDepth(1.0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBindVertexArray(0);

	glUseProgram(0);

	/* Set up meter module */
	g_y_offset = 25.0;
	meter_init(&g_viz_meters, screen_width, screen_height, 20, meter_ogl_renderer);
	struct widget_meter_style wstyles = {.width = 200, .height = 20, .padding = 2.0};
	widget_meter widgets[] = {
		{
			.name = "Num Buckets", .x = 5.0, .y = 0, .min = 1.0, .max = 512.0, .value = getglob(L, "num_buckets", 128.0),
			.callback = visualizer_meter_callback, .target = &g_num_buckets, .always_snap = true,
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "dB Muliplier", .x = 5.0, .y = 0, .min = 1.0, .max = 100.0, .value = getglob(L, "db_multiplier", 20.0),
			.callback = visualizer_meter_callback, .target = &g_db_multiplier, 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "dB Divisor", .x = 5.0, .y = 0, .min = 1.0, .max = 3.0, .value = getglob(L, "db_divisor", 1.0),
			.callback = visualizer_meter_callback, .target = &g_db_divisor, 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Bar Width", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0, .value = getglob(L, "bar_width", 2.0),
			.callback = visualizer_meter_callback, .target = &tweaks[1], 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Bar Spacing", .x = 5.0, .y = 0, .min = 0.0, .max = 30.0, .value = getglob(L, "bar_spacing", 0.0),
			.callback = visualizer_meter_callback, .target = &tweaks[2], 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Bar Min Height", .x = 5.0, .y = 0, .min = 0.0, .max = 100.0, .value = getglob(L, "bar_min_height", 0.0),
			.callback = visualizer_meter_callback, .target = &tweaks[3], 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Style", .x = 5.0, .y = 0, .min = 0.0, .max = NUM_VIZ_STYLES - 1, .value = g_viz_style,
			.callback = visualizer_style_callback,
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {65, 65, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Circle Inner Radius", .x = 5.0, .y = 0, .min = 0.0, .max = 256.0, .value = getglob(L, "inner_radius", 0.0),
			.target = &tweaks[4], 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {65, 65, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Circle Rotation", .x = 5.0, .y = 0, .min = 0.0, .max = 1.0, .value = getglob(L, "circle_rotation", 0.0),
			.target = &tweaks[5], 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {65, 65, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Circle Fraction", .x = 5.0, .y = 0, .min = 0.0, .max = 1.0, .value = getglob(L, "circle_fraction", 1.0),
			.target = &tweaks[6], 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {65, 65, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "Bar Power", .x = 5.0, .y = 0, .min = 0.0, .max = 128.0, .value = getglob(L, "bar_power", 1.0),
			.target = &tweaks[7], 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {65, 65, 95, 255}, .font = {255, 255, 255}}
		},
	};
	init_viz_meters(&g_viz_meters, widgets, LENGTH(widgets), &g_y_offset);
	meter_label(&g_viz_meters, "Style", "%s %.0f");

	g_blank_seconds = getglob(L, "blank_seconds", 2.0);

	return 0;
}

void visualizer_scene_resize(float width, float height)
{
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	screen_width = width;
	screen_height = height;
	meter_resize_screen(&g_viz_meters, screen_width, screen_height);
	// visualizer_scene_render();
	// color_buffer_delete(obuffer);
	// obuffer = color_buffer_new(obuffer_width, obuffer_height);
	//make_projection_matrix(65, screen_width/screen_height, 0.1, 200, proj_mat);	
}

void visualizer_scene_deinit()
{
	glDeleteVertexArrays(1, &g_visualizer_ogl.vao);
	glDeleteBuffers(1, &g_visualizer_ogl.vbo);
	glDeleteProgram(g_visualizer_ogl.shader);
	SDL_FreeWAV(g_wav_buffer);
	free(cfg);
	meter_deinit(&g_viz_meters);
	color_buffer_delete(obuffer);
}

void get_timedata(kiss_fft_scalar *timedata, SDL_AudioSpec wav_spec, unsigned char *wav_buffer, uint32_t wav_length, float seconds, float blank_seconds)
{
	int bits_per_sample = (wav_spec.format & 0xff);
	int bytes_per_sample = bits_per_sample / 8 + (bits_per_sample % 8 ? 1 : 0);
	int stride = bytes_per_sample * wav_spec.channels;
	double trackhead = stride * wav_spec.freq * (seconds - blank_seconds);
	trackhead = trackhead - fmod(trackhead, stride);
	// printf("bits_per_sample: %d, bytes_per_sample: %d, trackhead: %d\n", bits_per_sample, bytes_per_sample, trackhead);
	for (int i = 0; i < nfft; i++) {
		int32_t sum = 0;
		int index = (int)trackhead + stride*i - nfft/2;
		index = index >= 0 ? index : 0;
		if (index >= 0 && index + bytes_per_sample < wav_length) {
			for (int j = 0; j < bytes_per_sample; j++) {
				if (index + j < wav_length)
					sum |= (wav_buffer[index + j] << (8 * j));
			}
			timedata[i] = (kiss_fft_scalar)((double)sum/(INT32_MAX/2)) * hann(i, nfft);
		} else {
			timedata[i] = 0;
		}
	}
}

void make_buckets(float *buckets, int num_buckets, float bucket_width, kiss_fft_cpx *freqdata)
{
	for (int i = 0; i < num_buckets; i++) {
		buckets[i] = 0;
		for (int j = 0; j < bucket_width; j++) {
			kiss_fft_cpx f = freqdata[(int)(i * bucket_width + j)];
			buckets[i] += sqrt(f.r*f.r + f.i*f.i);
		}
		buckets[i] /= bucket_width;
	}
}

void load_buckets_into_texture_buf(float *buckets, int num_buckets, char *tex_buf, int tex_w, int tex_h)
{
	for (int h = 0; h < tex_h; h++) {
		for (int w = 0; w < tex_w; w++) {
			float val = buckets[(int)((float)w / tex_w / 2 * num_buckets)];
			float db_val = g_db_multiplier*log10(val) / g_db_divisor;
			tex_buf[tex_w * h + w] = fmax(db_val, 0.0);
		}
	}
}

void visualizer_scene_update(float dt)
{
	static float seconds = 0;
	
	Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
	bool button = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
	if (show_tweaks)
		button = !meter_mouse_relative(&g_viz_meters, mouse_x, mouse_y, button,
		key_state[SDL_SCANCODE_LSHIFT] || key_state[SDL_SCANCODE_RSHIFT],
		key_state[SDL_SCANCODE_LCTRL]  || key_state[SDL_SCANCODE_RCTRL]) && button;

	if (key_pressed(SDL_SCANCODE_SPACE))
	{
		visualizer_recording = !visualizer_recording;
		if (visualizer_recording) {
			seconds = 0;
			printf("Starting to record!\n");
			char *cmd = getglobstr(L, "recording_cmd", "output_error.txt");
			visualizer_file = popen(cmd, "w");
			free(cmd);
			if (!visualizer_file)
				printf("Could not open ffmpeg file.\n");
			visualizer_file_buffer = malloc(sizeof(int) * obuffer_width * obuffer_height);
			if (!visualizer_file_buffer)
				printf("Could not allocate memory.\n");

			SDL_CloseAudioDevice(g_dev_id);
		}
		else {
			printf("Stopping recording!\n");
			pclose(visualizer_file);
			visualizer_file = NULL;
			free(visualizer_file_buffer);
			visualizer_file_buffer = NULL;
		}
	}

	if (key_pressed(SDL_SCANCODE_TAB))
		show_tweaks = !show_tweaks;

	if (key_state[SDL_SCANCODE_M] && buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		g_offset_x = mouse_x;
		g_offset_y = screen_height - mouse_y;
	}


	get_timedata(g_timedata, g_wav_spec, g_wav_buffer, g_wav_length, seconds, visualizer_recording ? g_blank_seconds : 0.0);

	kiss_fftr(cfg, g_timedata, g_freqdata);

	float buckets[(int)g_num_buckets];
	float bucket_width = nfft/8.0/(int)g_num_buckets;
	make_buckets(buckets, LENGTH(buckets), bucket_width, g_freqdata);
	load_buckets_into_texture_buf(buckets, LENGTH(buckets), g_tex_buf, g_tex_w, g_tex_h);

	// if (visualizer_recording)
		seconds += 1.0 / 60.0;

	glBindTexture(GL_TEXTURE_2D, g_visualizer_tweaks.frequencies);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, g_tex_w, g_tex_h, 0, GL_RED, GL_UNSIGNED_BYTE, g_tex_buf);
}

void visualizer_scene_render()
{
	// Draw to offscreen buffer.
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, obuffer.fbo);
	glViewport(0, 0, obuffer_width, obuffer_height);
	glBindVertexArray(g_visualizer_ogl.vao);
	glUseProgram(g_visualizer_ogl.shader);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	CHECK_ERRORS()
	//Draw in wireframe if 'z' is held down.
	if (key_state[SDL_SCANCODE_Z])
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	CHECK_ERRORS()
	glUniform2f(g_visualizer_ogl.iTime, SDL_GetTicks() / 1000.0, accum_frames);
	glUniform2f(g_visualizer_ogl.iResolution, obuffer_width, obuffer_height);
	glUniform4f(g_visualizer_ogl.iMouse, mouse_x, mouse_y, 0, 0);
	tweaks[0] = g_num_buckets;
	CHECK_ERRORS()
	glUniform4fv(g_visualizer_ogl.tweaks, 1, tweaks);
	glUniform4fv(g_visualizer_ogl.tweaks2, 1, tweaks + 4);
	glUniform1i(g_visualizer_ogl.style, g_viz_style);

	glBindBuffer(GL_ARRAY_BUFFER, g_visualizer_ogl.vbo);
	CHECK_ERRORS()

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_visualizer_tweaks.frequencies);
	glUniform1i(g_visualizer_ogl.frequencies, 0);
	CHECK_ERRORS()

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			
	checkErrors("After draw");

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, obuffer.fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, screen_width, screen_height);
	glClearColor(visuals_overflowing ? 0.6 : 0.2, visualizer_recording ? 0.6 : 0.2, 0.2, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glBlitFramebuffer(0, 0, obuffer_width, obuffer_height, g_offset_x, g_offset_y, obuffer_width + g_offset_x, obuffer_height + g_offset_y, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	if (visualizer_recording && visualizer_file_buffer && visualizer_file) {
		glReadPixels(0, 0, obuffer_width, obuffer_height, GL_RGBA, GL_UNSIGNED_BYTE, visualizer_file_buffer);
		fwrite(visualizer_file_buffer, sizeof(int)*obuffer_width*obuffer_height, 1, visualizer_file);
	}

	if (show_tweaks)
		meter_draw_all(&g_viz_meters);
	checkErrors("After meter_draw_all");
	glBindVertexArray(0);
}
