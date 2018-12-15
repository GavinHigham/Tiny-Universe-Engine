#include "visualizer_scene.h"
#include "scene.h"
#include "glsw/glsw.h"
#include "glsw_shaders.h"
#include "macros.h"
#include "math/utility.h"
#include "drawf.h"
#include "input_event.h"
//#include "space/triangular_terrain_tile.h"
#include "configuration/lua_configuration.h"
#include "trackball/trackball.h"
#include "meter/meter.h"
#include "meter/meter_ogl_renderer.h"
#include "deferred_framebuffer.h"
#include "kiss_fftr.h"

#include <glla.h>
#include <GL/glew.h>
#include <stdio.h>
#include <assert.h>
//#include <SDL2_image/SDL_image.h>

/* Implementing scene "interface" */

SCENE_IMPLEMENT(visualizer);

static float screen_width = 640, screen_height = 480;
static float obuffer_width = 1920, obuffer_height = 1080;
static int mouse_x = 0, mouse_y = 0;
static struct trackball visualizer_trackball;
static struct color_buffer obuffer;
static int accum_frames = 0;
static float tweaks[8] = {1, 1, 1, 1};

/* Recording file */
FILE *visualizer_file;
int *visualizer_file_buffer = NULL;
static bool visualizer_recording = false;

/* Lua Config */
extern lua_State *L;

/* OpenGL Variables */

static GLint POS_ATTR = 1;
static struct {
	GLuint RESOLUTION, MOUSE, TIME, TWEAKS, TWEAKS2, TEX;
} UNIF;
static GLuint SHADER, VAO, VBO;
static GLfloat vertices[] = {-1, -1, 1, -1, -1, 1, 1, 1};

/* Texture and audio */
GLuint tex_handle;
GLenum tex_fmt = GL_RG8UI;
const int tex_w = 2048, tex_h = 2, texel_size = 2;
char tex_data[tex_w*tex_h*texel_size] = {0};
unsigned char *wav_buffer = NULL;
uint32_t wav_length;

SDL_AudioSpec wav_spec = {0}, audio_spec = {0};
int nfft = 16384;//4096; //2^14
kiss_fftr_cfg cfg;
kiss_fft_scalar *timedata;
kiss_fft_cpx    *freqdata;
float g_num_buckets, g_db_multiplier, g_db_divisor, g_blank_seconds;

static float hann(int n, int N)
{
	float s = sin((M_PI*n)/(N-1));
	return s*s;
}

static void visualizer_meter_callback(char *name, enum meter_state state, float value, void *context)
{
	// clear_accum = true;
}

int visualizer_scene_init()
{
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	/* Shader initialization */
	glswInit();
	glswSetPath("shaders/glsw/", ".glsl");
	glswAddDirectiveToken("glsl330", "#version 330");

	char *vsh_key = getglobstr(L, "visualizer_vsh_key", "");
	char *fsh_key = getglobstr(L, "visualizer_fsh_key", "");

	GLuint shader[] = {
		glsw_shader_from_keys(GL_VERTEX_SHADER, "versions.glsl330", vsh_key),
		glsw_shader_from_keys(GL_FRAGMENT_SHADER, "versions.glsl330", fsh_key),
	};
	SHADER = glsw_new_shader_program(shader, LENGTH(shader));
	glswShutdown();
	free(vsh_key);
	free(fsh_key);

	if (!SHADER) {
		visualizer_scene_deinit();
		return -1;
	}

	/* Retrieve uniform variable handles */

	UNIF.RESOLUTION  = glGetUniformLocation(SHADER, "iResolution");
	UNIF.MOUSE       = glGetUniformLocation(SHADER, "iMouse");
	UNIF.TIME        = glGetUniformLocation(SHADER, "iTime");
	UNIF.TWEAKS      = glGetUniformLocation(SHADER, "tweaks");
	UNIF.TWEAKS2     = glGetUniformLocation(SHADER, "tweaks2");
	UNIF.TEX         = glGetUniformLocation(SHADER, "frequencies");

	checkErrors("After getting uniform handles");
	glUseProgram(SHADER);

	/* Vertex data */

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	/* Vertex attributes */

	glEnableVertexAttribArray(POS_ATTR);
	glVertexAttribPointer(POS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, false);
	checkErrors("After pos attr");

	/* Audio */
	char *wav_file = getglobstr(L, "wav_filename", "");

	/* Load the WAV */
	if (SDL_LoadWAV(wav_file, &wav_spec, &wav_buffer, &wav_length) == NULL) {
		fprintf(stderr, "Could not open %s: %s\n", wav_file, SDL_GetError());
	} else {
		/* Do stuff with the WAV data, and then... */
		printf("freq: %d, fmt: %x, channels: %d, samples: %d, length %d\n", wav_spec.freq, wav_spec.format, wav_spec.channels, wav_spec.samples, wav_length);

		SDL_AudioSpec want, have;
		SDL_AudioDeviceID dev;

		// SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
		// want.freq = 48000;
		// want.format = AUDIO_F32;
		// want.channels = 2;
		// want.samples = 4096;
		// want.callback = NULL; /* you wrote this function elsewhere -- see SDL_AudioSpec for details */

		dev = SDL_OpenAudioDevice(NULL, 0, &wav_spec, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
		if (dev == 0) {
			SDL_Log("Failed to open audio: %s", SDL_GetError());
		} else {
			if (have.format != wav_spec.format) { /* we let this one thing change. */
				SDL_Log("We didn't get requested audio format.");
			}
			// SDL_QueueAudio(dev, wav_buffer, wav_length);
			// SDL_PauseAudioDevice(dev, 0); /* start audio playing. */
			// SDL_Delay(5000); /* let the audio callback play some sound for 5 seconds. */
			// SDL_CloseAudioDevice(dev);
		}
	}
	free(wav_file);

	cfg = kiss_fftr_alloc(nfft, 0, NULL, NULL);
	timedata = malloc(nfft*sizeof(kiss_fft_scalar));
	freqdata = malloc(nfft*sizeof(kiss_fft_cpx));


	/* Texture creation */

	glGenTextures(1, &tex_handle);
	glBindTexture(GL_TEXTURE_2D, tex_handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tex_w, tex_h, 0, GL_RED, GL_UNSIGNED_BYTE, tex_data);

	/* Offscreen Buffer */
	obuffer_width = getglob(L, "recording_width", 1920);
	obuffer_height = getglob(L, "recording_height", 1080);
	obuffer = color_buffer_new(obuffer_width, obuffer_height);

	/* Misc. OpenGL bits */

	glClearDepth(1.0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBindVertexArray(0);

	glUseProgram(0);

	/* Set up meter module */
	float y_offset = 25.0;
	meter_init(screen_width, screen_height, 20, meter_ogl_renderer);
	struct widget_meter_style wstyles = {.width = 200, .height = 20, .padding = 2.0};
	struct widget_meter_color tweak_colors = {.fill = {79, 150, 167, 255}, .border = {37, 95, 65, 255}, .font = {255, 255, 255, 255}};
	struct widget_meter_color bulge_colors = {.fill = {179, 95, 107, 255}, .border = {37, 95, 65, 255}, .font = {255, 255, 255, 255}};
	widget_meter widgets[] = {
		{
			.name = "Num Buckets", .x = 5.0, .y = 0, .min = 1.0, .max = 2048.0, .value = getglob(L, "num_buckets", 2048.0),
			.callback = visualizer_meter_callback, .target = &g_num_buckets, 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "dB Muliplier", .x = 5.0, .y = 0, .min = 1.0, .max = 100.0, .value = getglob(L, "db_multiplier", 20.0),
			.callback = visualizer_meter_callback, .target = &g_db_multiplier, 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		{
			.name = "dB Divisor", .x = 5.0, .y = 0, .min = 1.0, .max = 1024.0, .value = getglob(L, "db_divisor", 1.0),
			.callback = visualizer_meter_callback, .target = &g_db_divisor, 
			.style = wstyles, .color = {.fill = {187, 187, 187, 255}, .border = {95, 95, 95, 255}, .font = {255, 255, 255}}
		},
		// {
		// 	.name = "Rotation", .x = 5.0, .y = 0, .min = 0.0, .max = 1000.0, .value = getglob(L, "visualizer_rotation", 400.0),
		// 	.callback = visualizer_meter_callback, .target = &g_rotation,
		// 	.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		// },
		// {
		// 	.name = "Arm Width", .x = 5.0, .y = 0, .min = 0.0, .max = 32.0, .value = getglob(L, "arm_width", 2.0),
		// 	.callback = visualizer_meter_callback, .target = &tweaks[5],
		// 	.style = wstyles, .color = {.fill = {79, 79, 207, 255}, .border = {47, 47, 95, 255}, .font = {255, 255, 255, 255}}
		// },
		// {
		// 	.name = "Noise Scale", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0, .value = getglob(L,"noise_scale", 9.0),
		// 	.callback = visualizer_meter_callback, .target = &tweaks[0],
		// 	.style = wstyles, .color = tweak_colors
		// },
		// {
		// 	.name = "Noise Influence", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0, .value = getglob(L,"noise_influence", 9.0),
		// 	.callback = visualizer_meter_callback, .target = &tweaks[1],
		// 	.style = wstyles, .color = tweak_colors
		// },
		// {
		// 	.name = "Diffuse Step Distance", .x = 5.0, .y = 0, .min = -10.0, .max = 10.0, .value = getglob(L,"light_step_distance", 2.0),
		// 	.callback = visualizer_meter_callback, .target = &tweaks[3],
		// 	.style = wstyles, .color = tweak_colors
		// },
		// {
		// 	.name = "Diffuse Intensity", .x = 5.0, .y = 0, .min = 0.000001, .max = 1.0, .value = getglob(L,"diffuse_intensity", 1.0),
		// 	.callback = visualizer_meter_callback, .target = &tweaks[4],
		// 	.style = wstyles, .color = tweak_colors
		// },
		// {
		// 	.name = "Emission Strength", .x = 5.0, .y = 0, .min = 0.000001, .max = 15.0, .value = getglob(L,"emission_strength", 1.0),
		// 	.callback = visualizer_meter_callback, .target = &tweaks[7],
		// 	.style = wstyles, .color = tweak_colors
		// },
		// {
		// 	.name = "Bulge Mask Radius", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0, .value = getglob(L,"bulge_mask_radius", 9.0),
		// 	.callback = visualizer_meter_callback, .target = &g_bulge_mask_radius,
		// 	.style = wstyles, .color = bulge_colors
		// },
		// {
		// 	.name = "Bulge Mask Power", .x = 5.0, .y = 0, .min = 0.0, .max = 8.0, .value = getglob(L,"bulge_mask_power", 1.0),
		// 	.callback = visualizer_meter_callback, .target = &tweaks[6],
		// 	.style = wstyles, .color = bulge_colors
		// },
		// {
		// 	.name = "Bulge Height", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0, .value = getglob(L,"bulge_height", 20.0),
		// 	.callback = meter_clear_accum_callback, .target = &g_bulge_height,
		// 	.style = wstyles, .color = bulge_colors
		// },
		// {
		// 	.name = "Bulge Width", .x = 5.0, .y = 0, .min = 0.0, .max = 50.0, .value = getglob(L,"bulge_width", 10.0),
		// 	.callback = meter_clear_accum_callback, .target = &g_bulge_width,
		// 	.style = wstyles, .color = bulge_colors
		// },
		// {
		// 	.name = "Spiral Density", .x = 5.0, .y = 0, .min = 0.0, .max = 4.0, .value = getglob(L,"visualizer_density", 1.0),
		// 	.callback = meter_clear_accum_callback, .target = &tweaks[2],
		// 	.style = wstyles, .color = tweak_colors
		// },
	};
	for (int i = 0; i < LENGTH(widgets); i++) {
		widget_meter *w = &widgets[i];
		meter_add(w->name, w->style.width, w->style.height, w->min, w->value, w->max);
		meter_target(w->name, w->target);
		meter_position(w->name, w->x, w->y + y_offset);
		meter_callback(w->name, w->callback, w->callback_context);
		meter_style(w->name, w->color.fill, w->color.border, w->color.font, w->style.padding);
		y_offset += 25;
	}

	g_blank_seconds = getglob(L, "blank_seconds", 2.0);

	return 0;
}

void visualizer_scene_resize(float width, float height)
{
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	screen_width = width;
	screen_height = height;
	meter_resize_screen(screen_width, screen_height);

	// color_buffer_delete(obuffer);
	// obuffer = color_buffer_new(obuffer_width, obuffer_height);
	//make_projection_matrix(65, screen_width/screen_height, 0.1, 200, proj_mat);	
}

void visualizer_scene_deinit()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(SHADER);
	SDL_FreeWAV(wav_buffer);
	free(cfg);
	meter_deinit();
	color_buffer_delete(obuffer);
}

void visualizer_scene_update(float dt)
{
	// eye_frame.t += (vec3){
	// 	key_state[SDL_SCANCODE_D] - key_state[SDL_SCANCODE_A],
	// 	key_state[SDL_SCANCODE_E] - key_state[SDL_SCANCODE_Q],
	// 	key_state[SDL_SCANCODE_S] - key_state[SDL_SCANCODE_W],
	// } * 0.15;
	static float seconds = 0;
	
	Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
	bool button = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
	int meter_clicked = meter_mouse(mouse_x, mouse_y, button);
	button = button && !meter_clicked;
	// int scroll_x = input_mouse_wheel_sum.wheel.x, scroll_y = input_mouse_wheel_sum.wheel.y;
	// if (trackball_step(&visualizer_trackball, mouse_x, mouse_y, button, scroll_x, scroll_y))
	// 	clear_accum = true;

	bool record_key_pressed = false;
	static bool record_key_down_last_frame = false;
	if (key_state[SDL_SCANCODE_SPACE]) {
		if (!record_key_down_last_frame)
			record_key_pressed = true;
		record_key_down_last_frame = true;
	} else {
		record_key_down_last_frame = false;
	}
	if (record_key_pressed) {
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
		}
		else {
			printf("Stopping recording!\n");
			pclose(visualizer_file);
			visualizer_file = NULL;
			free(visualizer_file_buffer);
			visualizer_file_buffer = NULL;
		}
	}

	int bits_per_sample = (wav_spec.format & 0xff);
	int bytes_per_sample = bits_per_sample / 8 + (bits_per_sample % 8 ? 1 : 0);
	int stride = bytes_per_sample * wav_spec.channels;
	double trackhead = stride * wav_spec.freq * (seconds - g_blank_seconds);
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
	kiss_fftr(cfg, timedata, freqdata);
	float buckets[(int)g_num_buckets];
	int bucket_width = nfft/8/(int)g_num_buckets;
	for (int i = 0; i < LENGTH(buckets); i++) {
		buckets[i] = 0;
		for (int j = 0; j < bucket_width; j++) {
			kiss_fft_cpx f = freqdata[i * bucket_width + j];
			// buckets[i] = fmax(buckets[i], sqrt(f.r*f.r + f.i*f.i));
			buckets[i] += sqrt(f.r*f.r + f.i*f.i);
		}
		buckets[i] /= bucket_width;
	}

	// static float max_seen = 0;
	// static float min_seen = INFINITY;
	for (int h = 0; h < tex_h; h++) {
		for (int w = 0; w < tex_w; w++) {
			float val = buckets[(int)((float)w / tex_w / 2 * g_num_buckets)];
			float db_val = g_db_multiplier*log10(val) / g_db_divisor;
			// if (db_val > max_seen) {
			// 	max_seen = db_val;
			// 	printf("New max: %f\n", max_seen);
			// }
			// if (db_val < min_seen) {
			// 	min_seen = db_val;
			// 	printf("New min: %f\n", min_seen);
			// }
			tex_data[tex_w * h + w] = fmax(db_val, 0.0);
		}
	}

	// if (visualizer_recording)
		seconds += 1.0 / 60.0;

	glBindTexture(GL_TEXTURE_2D, tex_handle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tex_w, tex_h, 0, GL_RED, GL_UNSIGNED_BYTE, tex_data);
}

void visualizer_scene_render()
{
	// Draw to offscreen buffer.
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, obuffer.fbo);
	glViewport(0, 0, obuffer_width, obuffer_height);
	glBindVertexArray(VAO);
	glUseProgram(SHADER);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//Draw in wireframe if 'z' is held down.
	if (key_state[SDL_SCANCODE_Z])
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


	glUniform2f(UNIF.TIME, SDL_GetTicks() / 1000.0, accum_frames);
	glUniform2f(UNIF.RESOLUTION, obuffer_width, obuffer_height);
	glUniform4f(UNIF.MOUSE, mouse_x, mouse_y, 0, 0);
	glUniform4fv(UNIF.TWEAKS, 1, tweaks);
	glUniform4fv(UNIF.TWEAKS2, 1, tweaks + 4);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_handle);
	glUniform1i(UNIF.TEX, 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			
	checkErrors("After draw");

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, obuffer.fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, screen_width, screen_height);
	glBlitFramebuffer(0, 0, obuffer_width, obuffer_height, 0, 0, screen_width, screen_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	if (visualizer_recording && visualizer_file_buffer && visualizer_file) {
		glReadPixels(0, 0, obuffer_width, obuffer_height, GL_RGBA, GL_UNSIGNED_BYTE, visualizer_file_buffer);
		fwrite(visualizer_file_buffer, sizeof(int)*obuffer_width*obuffer_height, 1, visualizer_file);
	}

	meter_draw_all();
	checkErrors("After meter_draw_all");
	glBindVertexArray(0);
}
