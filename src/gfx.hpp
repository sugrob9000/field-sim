#pragma once

#include "gl.hpp"

namespace gfx {
struct Config {
	bool debug = false;
	unsigned msaa_samples = 0;  // 0 for no MSAA
	unsigned particles_x = 0, particles_y = 0; // 0 for auto
};

void init (unsigned res_x, unsigned res_y, Config);
void deinit ();

void handle_sdl_event (const SDL_Event&);
void present_frame ();

void fieldviz_update ();
void fieldviz_draw (bool should_clear);
}
