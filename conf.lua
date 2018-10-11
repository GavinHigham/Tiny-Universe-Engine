--Window config values
screen_width  = 800
screen_height = 600
screen_title = "Simple OpenGL C (k) Engine"
fullscreen = false
grab_mouse = false

--ffmpeg recording
ffmpeg_cmd = "ffmpeg -r 15 -f rawvideo -pix_fmt rgba -s " .. screen_width .. "x" .. screen_height .. " -i - -threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip output.mp4"

default_scene = "space_scene" --"twotri_scene" --"icosphere_scene"
-- default_scene = "twotri_scene"

--proctri_scene.c config values
proctri_tex = "grass.png"
tex_scale = 4
gpu_tiles = false
num_tile_rows = 80
gen_solar_systems = true

--twotri_scene.c config values
twotri_vsh_key = "spiral.vertex.GL33"
twotri_fsh_key = "spiral.fragment.GL33"
max_accum_frames = 180
accumulate = true
spiral_rotation = 300.0
noise_scale = 3.6
noise_influence = 4.7
spiral_density = 2.8
bulge_mask_radius = 12.0
bulge_height = 14.0
bulge_width = 23.0