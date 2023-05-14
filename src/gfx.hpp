#pragma once

#include "gl.hpp"

namespace gfx {
struct Config {
	unsigned screen_res_x = 1560, screen_res_y = 960;
	bool debug = false;
	unsigned msaa_samples = 0;  // 0 for no MSAA
	unsigned particles_x = 0, particles_y = 0;  // 0 for auto
	unsigned particle_lifetime = 200;
};

void init (const Config&);
void deinit ();

void handle_sdl_event (const SDL_Event&);
void present_frame ();

void fieldviz_update ();
void fieldviz_draw (bool should_clear);
}
