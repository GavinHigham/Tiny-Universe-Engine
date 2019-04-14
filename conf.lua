--Window config values
screen_width  = 800
screen_height = 600
screen_title = "Simple OpenGL C (k) Engine"
fullscreen = false
grab_mouse = false

default_scene = "space" --"twotri" --"proctri"
-- default_scene = "visualizer"
-- default_scene = "spiral"

--ffmpeg recording
ffmpeg_cmd = "ffmpeg -r 15 -f rawvideo -pix_fmt rgba -s " .. screen_width .. "x" .. screen_height .. " -i - -threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip output.mp4"

--proctri_scene.c config values
proctri_tex = "grass.png"
tex_scale = 4
gpu_tiles = false
num_tile_rows = 80
gen_solar_systems = false

--twotri_scene.c config values
spiral_vsh_key = "spiral.vertex.GL33"
spiral_fsh_key = "spiral.fragment.GL33"
cubemap_width = 512
cubemap_mode = false
max_accum_frames = 20
accumulate = true
galaxy_defaults = {
	arm_width = 2.0,
	rotation = 300.0,	
	noise_scale = 4.7,
	noise_strength = 4.7,
	spiral_density = 2.8,
	bulge_mask_radius = 12.0,
	bulge_height = 14.0,
	bulge_width = 23.0,
	-- samples = 50,
	-- brightness = 100
}

--visualizer_scene.c config values
-- num_buckets = 2048
db_multiplier = 60
bar_width = 8
bar_min_height = 3
bar_spacing = 4
num_buckets = 42
inner_radius = 60
visualizer_vsh_key = "visualizer.vertex.GL33"
visualizer_fsh_key = "visualizer.fragment.GL33"
visualizer_style = 1
visualizer_circle_width = 512

-- wav_filename = "Bbibbi_cover_ver1.wav"
-- wav_filename = "Love Scenario_cover.wav"
-- wav_filename = "Greenseomusic_Oceanwide.wav"
-- wav_filename = "Twit_cover.wav"
-- wav_filename = "Greenseomusic_Happy New Start.wav"
-- wav_filename = "DallaDalla_cover.wav"
wav_filename = "SpringDay_cover_1.wav"
if (default_scene == "visualizer") then
	recording_width, recording_height = 512, 512
	-- screen_width, screen_height = recording_width + 100, recording_height + 100
	recording_cmd = "ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s " .. recording_width .. "x" .. recording_height .. " -i - -threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip " .. wav_filename .. ".mp4"
end


