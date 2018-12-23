--Window config values
screen_width  = 800
screen_height = 600
screen_title = "Simple OpenGL C (k) Engine"
fullscreen = false
grab_mouse = false

--ffmpeg recording
ffmpeg_cmd = "ffmpeg -r 15 -f rawvideo -pix_fmt rgba -s " .. screen_width .. "x" .. screen_height .. " -i - -threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip output.mp4"
recording_width, recording_height = 1920, 1080
recording_cmd = "ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s " .. recording_width .. "x" .. recording_height .. " -i - -threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip recording.mp4"


default_scene = "icosphere_scene" --"twotri_scene" --"space_scene"
-- default_scene = "visualizer"
-- default_scene = "spiral"

--proctri_scene.c config values
proctri_tex = "grass.png"
tex_scale = 4
gpu_tiles = false
num_tile_rows = 80
gen_solar_systems = true

--twotri_scene.c config values
spiral_vsh_key = "spiral.vertex.GL33"
spiral_fsh_key = "spiral.fragment.GL33"
max_accum_frames = 180
accumulate = true
spiral_rotation = 300.0
noise_scale = 3.6
noise_strength = 4.7
spiral_density = 2.8
bulge_mask_radius = 12.0
bulge_height = 14.0
bulge_width = 23.0

--visualizer_scene.c config values
num_buckets = 2048
db_multiplier = 60
visualizer_vsh_key = "visualizer.vertex.GL33"
visualizer_fsh_key = "visualizer.fragment.GL33"
wav_filename = "Bbibbi_cover_ver1.wav"
-- wav_filename = "Love Scenario_cover.wav"