--Window config values
screen_width  = 1024
screen_height = 768
fullscreen = false
grab_mouse = false
allow_highdpi = false --Experimental, does not handle window resize.

-- default_scene = "space" --"twotri" --"proctri"
-- default_scene = "visualizer"
-- default_scene = "spiral"
-- default_scene = "spawngrid"
default_scene = "atmosphere"
-- default_scene = "universe"
-- default_scene = 'lua'
screen_title = ('Current Scene: "%s"'):format(manual_scene or default_scene)

lua_scene = 'luaengine/scripts/proctri_scene'

--ffmpeg recording
ffmpeg_cmd = "ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s " .. screen_width .. "x" .. screen_height .. " -i - -threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip output.mp4"

--proctri_scene.c config values
proctri_tex = "grass.png"
tex_scale = 4
gpu_tiles = false
num_tile_rows = 80
gen_solar_systems = false

--twotri_scene.c config values
spiral_vsh_key = "spiral.vertex.GL33"
spiral_fsh_key = "spiral.fragment.GL33"
cubemap_width = 1024
cubemap_mode = false
max_accum_frames = 20
accumulate = true
galaxy_defaults = {
	arm_width = 2.85,
	rotation = 543.0,
	diameter = 80.0,
	noise_scale = 8.64,
	noise_strength = 4.7,
	disk_height = 5.24,
	spiral_density = 2.8,
	bulge_mask_radius = 9.69,
	bulge_mask_power = 3.43,
	bulge_width = 23.0,
	light_absorption_r = 0.16,
	light_absorption_g = 0.19,
	light_absorption_b = 0.17,
	-- samples = 50,
	-- brightness = 100
}

atmosphere_defaults = {

}

spawngrid_defaults = {

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
visualizer_style = 0
visualizer_circle_width = 512

local info = [[
Controls:
Hit space to begin recording, hit it again to finish and save the video
Hold M and drag to move the display window
Hold CTRL while dragging a slider to snap to whole numbers (1, 2, 3 etc.)
Hold Shift while dragging a slider for fine adjustment
Hold CTRL and Shift while dragging a slider to snap to tenths of a number (1.10, 1.20, 1.30, etc.)

If the background appears red, it means the bar visualization is too wide for the recording window -
 make the buckets narrower, decrease "num buckets", or decrease spacing until the visualization fits.
]]

-- wav_filename = "Bbibbi_cover_ver1.wav"
-- wav_filename = "Love Scenario_cover.wav"
-- wav_filename = "Greenseomusic_Oceanwide.wav"
-- wav_filename = "Twit_cover.wav"
-- wav_filename = "Greenseomusic_Happy New Start.wav"
-- wav_filename = "DallaDalla_cover.wav"
wav_filename = "SpringDay_cover_1.wav"
if (default_scene == "visualizer") then
	recording_width, recording_height = 800, 400
	wav_filename = io.popen([[osascript -e 'POSIX path of {choose file with prompt "Please choose a WAV file:" of type {"wav"}}']]):read('a'):gsub('^%s+', ''):gsub('%s+$', '')
	local path, filename, extension = wav_filename:match("^(.*/)(.*)(%.[wW][aA][vV])")
	path = 'Macintosh HD:' .. path:gsub('/', ':')
	mp4_filename = io.popen([[osascript -e 'POSIX path of {choose file name with prompt "Save the video as:" default name "]]..filename..[[.mp4" default location alias "]]..path..[["}']]):read('a'):gsub('^%s+', ''):gsub('%s+$', '')

	-- screen_width, screen_height = recording_width + 100, recording_height + 100
	recording_cmd = data_path .. 'ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s ' .. recording_width .. 'x' .. recording_height .. ' -i - -threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip "' .. mp4_filename .. '"'
end