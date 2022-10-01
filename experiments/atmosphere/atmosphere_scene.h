#ifndef ATMOSPHERE_SCENE_H
#define ATMOSPHERE_SCENE_H

#include <stdbool.h>
extern struct game_scene atmosphere_scene;
struct atmosphere_tweaks {
	float planet_radius;
	float fov;
	float focal_length;
	float scene_time;
	float atmosphere_height;
	float samples_ab, samples_cp;
	float p_and_p;
	float air_b[3];
	float scale_height;
	float planet_scale;
	bool show_tweaks;
};

#endif